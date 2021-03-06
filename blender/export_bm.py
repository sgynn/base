
from math import radians
from math import sqrt

import bpy
from mathutils import *

def StrValues(a): return ' '.join(str(round(x,6)) for x in a)

class BMExporter:
    def __init__(self, Config, context):
        self.Config = Config
        self.context = context

        self.Log("Begin verbose logging ----------\n")

        self.File = File(self.Config.filepath)

        self.Log("Generating object lists for export...")
        if self.Config.SelectedOnly:
            ExportList = list(self.context.selected_objects)
        else:
            ExportList = list(self.context.scene.objects)

        # Add linked skeletons to export list
        exportAnimations = self.Config.ExportAnimations or self.Config.ExportAllAnimations
        if self.Config.ExportArmatureBones or exportAnimations:
            for obj in ExportList:
                if obj.type == 'MESH':
                    arm = obj.find_armature()
                    if arm and arm not in ExportList:
                        ExportList.append(arm)

        # ExportMap maps Blender objects to ExportObjects
        ExportMap = {}
        for Object in ExportList:
            if Object.type == 'EMPTY':
                ExportMap[Object] = EmptyExportObject(self.Config, self, Object)
            elif Object.type == 'MESH' and self.Config.ExportMeshes:
                ExportMap[Object] = MeshExportObject(self.Config, self, Object)
            elif Object.type == 'ARMATURE':
                ExportMap[Object] = ArmatureExportObject(self.Config, self, Object)

        self.ExportList = Util.SortByNameField(ExportMap.values())

        for Item in self.ExportList:
            self.Log("  {} {}".format(Item.BlenderObject.type, Item.name))


        # Determine each object's children from the pool of ExportObjects
        for Object in ExportMap.values():
            Children = Object.BlenderObject.children
            Object.Children = []
            for Child in Children:
                if Child in ExportMap:
                    Object.Children.append(ExportMap[Child])

        self.Log("Done")

        self.AnimationWriter = None
        if exportAnimations:
            # Collect all animated object data
            self.Log("Gathering animation data...")
            AnimationGenerators = self.GatherAnimationGenerators()
            self.AnimationWriter = AnimationWriter(self.Config, self, AnimationGenerators)
            self.Log("Done")

    # "Public" Interface

    def Export(self):
        self.Log("Exporting to {}".format(self.File.FilePath),
            MessageVerbose=False)

        # Export everything
        self.Log("Opening file...")
        self.File.Open()
        self.Log("Done")

        self.Log("Writing header...")
        self.WriteHeader()
        self.Log("Done")

        if self.Config.ExportLayout:
            self.WriteLayout()

        self.Log("Writing objects...")
        for Object in self.ExportList:
            Object.Write()
        self.Log("Done writing objects")

        if self.AnimationWriter is not None:
            self.Log("Writing animation...")
            self.AnimationWriter.WriteAnimations()
            self.Log("Done writing animation")

        self.WriteFooter();
        self.Log("Closing file...")
        self.File.Close()
        self.Log("Done")

    def Log(self, String, MessageVerbose=True):
        if self.Config.Verbose is True or MessageVerbose == False:
            print(String)

    # "Private" Methods

    def WriteHeader(self):
        self.File.Write("<?xml version='1.0'?>\n<model>\n")
        self.File.Indent();

    def WriteFooter(self):
        self.File.Unindent();
        self.File.Write("</model>\n")

    def WriteLayout(self):
        self.File.Write("<layout>\n")
        self.File.Indent()

        for obj in self.ExportList:
            o = obj.BlenderObject
            q = o.rotation_quaternion
            r = (q.w, q.x, q.z, q.y)
            p = (o.location.x, o.location.z, -o.location.y)
            s = "<object name='{}'".format(o.name);
            if o.location.to_tuple()!=(0,0,0): s += " position='{}'".format(StrValues(p))
            if r != (1,0,0,0):                 s += " orientation='{}'".format(StrValues(r))
            if o.scale.to_tuple()!=(1,1,1):    s += " scale='{}'".format(StrValues(o.scale.xzy))
            if 'tag' in o:                     s += " tag='{}'".format(o['tag'])
            self.File.Write(s+"/>\n");

        self.File.Unindent()
        self.File.Write("</layout>\n")

    def GatherAnimationGenerators(self):
        Generators = []

        for obj in self.ExportList:
            if obj.BlenderObject.animation_data:
                # Collect actions
                actions = []
                animData = obj.BlenderObject.animation_data
                if animData.action:
                    actions.append(animData.action)

                # list actions from NLA
                if animData.nla_tracks and self.Config.ExportAllAnimations:
                    for track in animData.nla_tracks.values():
                        for strip in track.strips.values():
                            if strip.action and strip.action not in actions:
                                actions.append(strip.action)

                # ToDo: Handle un-referenced animations ?

                # If an object has an action, build its appropriate generator
                activeAction = animData.action
                for action in actions:
                    print('Action', action.name, ':', action.frame_range[1] - action.frame_range[0], "frames")

                    # Action needs to be active for this
                    animData.action = action
                    if obj.BlenderObject.type == 'ARMATURE':
                        gen = ArmatureAnimationGenerator( self.Config, action.name, obj )
                        Generators.append( gen )
                    else:
                        gen = GenericAnimationGenerator(self.Config, action.name, obj)
                        Generators.append( gen )

                animData.action = activeAction

        return Generators

# This class wraps a Blender object and writes its data to the file
class ExportObject: # Base class, do not use
    def __init__(self, Config, Exporter, BlenderObject):
        self.Config = Config
        self.Exporter = Exporter
        self.BlenderObject = BlenderObject

        self.name = self.BlenderObject.name # Simple alias
        self.Children = []

    def __repr__(self):
        return "[ExportObject: {}]".format(self.BlenderObject.name)

    def Write(self):
        pass

    def _WriteChildren(self):
        for Child in Util.SortByNameField(self.Children):
            Child.Write()

# Simple decorator implemenation for ExportObject.  Used by empty objects
class EmptyExportObject(ExportObject):
    def __init__(self, Config, Exporter, BlenderObject):
        ExportObject.__init__(self, Config, Exporter, BlenderObject)

    def __repr__(self):
        return "[EmptyExportObject: {}]".format(self.name)

# Mesh object implementation of ExportObject
class MeshExportObject(ExportObject):
    def __init__(self, Config, Exporter, BlenderObject):
        ExportObject.__init__(self, Config, Exporter, BlenderObject)

    def __repr__(self):
        return "[MeshExportObject: {}]".format(self.name)

    # "Public" Interface

    def Write(self):
        # Generate the export mesh
        self.Exporter.Log("Generating mesh for export...")
        Mesh = None

        if self.Config.ApplyTransform:
            bpy.ops.object.transform_apply(rotation=True, scale=True)

        if self.Config.ApplyModifiers:
            # Certain modifiers shouldn't be applied in some cases
            # Deactivate them until after mesh generation is complete

            DeactivatedModifierList = []

            # If we're exporting armature data, we shouldn't apply
            # armature modifiers to the mesh
            if self.Config.ExportSkinWeights:
                DeactivatedModifierList = [Modifier
                    for Modifier in self.BlenderObject.modifiers
                    if Modifier.type == 'ARMATURE' and \
                    Modifier.show_viewport]

            for Modifier in DeactivatedModifierList:
                Modifier.show_viewport = False

            Mesh = self.BlenderObject.to_mesh(self.Exporter.context.scene, True, 'PREVIEW')

            # Restore the deactivated modifiers
            for Modifier in DeactivatedModifierList:
                Modifier.show_viewport = True
        else:
            Mesh = self.BlenderObject.to_mesh(self.Exporter.context.scene, False, 'PREVIEW')
        self.Exporter.Log("Done")

        # SystemMatrix converts from right-handed, z-up to the target coordinate system
        yUp = self.Config.UpAxis == 'Y'
        leftHanded = self.Config.CoordinateSystem == 'RIGHT_HANDED'
        if leftHanded or yUp:
            transform = Matrix()
            if leftHanded:
                transform *= Matrix.Scale(-1, 4, Vector((0, 0, 1)))
            if yUp:
                transform *= Matrix.Rotation(radians(-90), 4, 'X')
            Mesh.transform(transform)
            Mesh.calc_normals()

        # Split meshes by materials
        if Mesh.materials:
            for Material in Mesh.materials:
                self.WriteMesh(Mesh, Material)
        else:
            self.WriteMesh(Mesh)

        # Cleanup
        bpy.data.meshes.remove(Mesh)

    # "Protected"

    # Vertex class to collect all data for a vertex. Also to check if a vertex needs duplicating
    class Vertex:
        def __init__(self, pos=None, normal=None, uv=None, colour=None, tangent=None):
            self.Position = pos
            self.Tangent = tangent
            self.Normal = normal
            self.Colour = colour
            self.UV = uv #mathUtils.Vector((0,0))

        def __eq__(self, other):
            if not self.Position == other.Position: return False
            #if not self.Tangent == other.Tangent: return False
            if not self.Normal == other.Normal: return False
            if not self.Colour == other.Colour: return False
            if not self.UV == other.UV: return False
            return True


    # Intermediate mesh class. Duplicates vertices that need duplicating due to normal or uv seams =============
    class IntermediateMesh:
        def __init__(self):
            pass


        def buildSimple(self, Mesh):
            self.Vertices = Mesh.vertices;
            self.Indices  = tuple( tuple(Polygon.vertices) for Polygon in Mesh.polygons )
            self.Map = self.Indices


        def build(self, Mesh, Index, useNormals, useUVs=False, useColours=False, useTangents=False):
            self.Vertices = []
            self.Indices = []
            self.Map = [[] for _ in range(0,len(Mesh.vertices))]    # array of empty arrays

            Mesh.calc_normals_split()
            UVs = Mesh.uv_layers.active.data if useUVs and Mesh.uv_layers else []
            if not Mesh.vertex_colors.active: useColours = False
            if not useNormals or not UVs: useTangents = False
            Colours = Mesh.vertex_colors.active.data if useColours else []

            # Colour alpha - see if there is a non-active layer called alpha
            Alphas = []
            if useColours:
                for col in Mesh.vertex_colors.items():
                    if not col[1].active and col[0].lower() == 'alpha':
                        Alphas = col[1].data

            wm = bpy.context.window_manager
            wm.progress_begin(0, len(Mesh.polygons))
            progress = 0;

            for Polygon in Mesh.polygons:
                progress = progress + 1
                wm.progress_update(progress)
                if Index == Polygon.material_index:
                    face = []
                    for i, Vertex in enumerate(Polygon.vertices):
                        index = -1
                        vx = MeshExportObject.Vertex( Mesh.vertices[Vertex].co )
                        loop = Polygon.loop_indices[i]
                        if useNormals: vx.Normal = Mesh.loops[ loop ].normal #if Polygon.use_smooth else Polygon.normal
                        if UVs: vx.UV = UVs[ loop ].uv
                        if Colours:
                            vx.Colour = Colours[ loop ].color
                            if Alphas: vx.Colour += (Alphas[ loop ].color[0],)

                        # y-up: y=z; z=-y; use setting. may be faster to preprocess mesh

                        # Find existing vertex
                        if self.Map[Vertex]:
                            for ix in self.Map[Vertex]:
                                if vx == self.Vertices[ix]:
                                    index = ix
                                    break

                        # Add new vertex
                        if index == -1:
                            index = len(self.Vertices)
                            self.Map[Vertex].append( index )
                            self.Vertices.append( vx )

                        # Add index to face
                        face.append( index )

                        # Add to face list - triangulate
                        if len(face) == 3:
                            if useTangents: self.calculateTangent(face, self.Vertices)
                            self.Indices.append(face)
                            face = face[:3:2]

            wm.progress_end()

        def crossProduct(self, a, b):
            return [ a[1]*b[2] - a[2]*b[1], a[2]*b[0]-a[0]*b[2], a[0]*b[1]-a[1]*b[0] ]

        def calculateTangent(self, face, vertices):
            va = vertices[ face[0] ]
            vb = vertices[ face[1] ]
            vc = vertices[ face[2] ]
            ab = [ vb.Position[0] - va.Position[0], vb.Position[1] - va.Position[1], vb.Position[2] - va.Position[2] ]
            ac = [ vb.Position[0] - va.Position[0], vc.Position[1] - va.Position[1], vc.Position[2] - va.Position[2] ]

            # project ab, ac onto face normal
            n = self.crossProduct( ab, ac )
            abn = ab[0] * n[0] + ab[1] * n[1] + ab[2] * n[2]
            acn = ac[0] * n[0] + ac[1] * n[1] + ac[2] * n[2]
            ab[0] -= n[0] * abn
            ab[1] -= n[1] * abn
            ab[2] -= n[2] * abn
            ac[0] -= n[0] * acn
            ac[1] -= n[1] * acn
            ac[2] -= n[2] * acn

            # texture coordinate deltas
            abu = vb.UV[0] - va.UV[0]
            abv = vb.UV[1] - va.UV[1]
            acu = vc.UV[0] - va.UV[0]
            acv = vc.UV[1] - va.UV[1]
            if acv*abu > abv*acu:
                acv = -acv
                abv = -abv

            # tangent
            tx = ac[0] * abv - ab[0] * acv
            ty = ac[1] * abv - ab[1] * acv
            tz = ac[2] * abv - ab[2] * acv

            # Normalise
            l = sqrt(tx*tx + ty*ty + tz * tz)
            if l != 0:
                tx = tx / l
                ty = ty / l
                tz = tz / l

            va.Tangent = (tx, ty, tz)
            vb.Tangent = (tx, ty, tz)
            vc.Tangent = (tx, ty, tz)


    # Needs to go in a separate section as these are global. Preferably at the top
    def WriteMaterials(self, Mesh):
        for Material in Mesh.materials:
            self.Exporter.File.Write("<material name='{}'>\n".format(Material.name))
            self.Exporter.File.Indent()

            Diffuse = list(Vector(Material.diffuse_color) * Material.diffuse_intensity)
            self.Exporter.File.Write("<diffuse r='{}' g='{}' b='{}' a='{}'>\n".format(Diffuse[0], Diffuse[1], Diffuse[2], Material.alpha))

            Shininess = 1000 * (Material.specular_hardness - 1.0) / 510
            Specular = list(Vector(Material.specular_color) * Material.specular_intensity)
            self.Exporter.File.Write("<specular r='{}' g='{}' b='{}' power='{}'>\n".format(Specular[0], Specular[1], Specular[2], Shininess))

            for Texture in Material.texture_slots:
                if Texture.texture.type == 'IMAGE' and getattr(Texture.image, "source", "") == 'FILE':
                    path = bpy.path.basename( Texture.image.filepath )
                    self.Exporter.File.Write("<texture file='{}'/>\n".format(path))

            self.Exporter.File.Unindent()
            self.Exporter.File.Write("</material>\n");


    def WriteMesh(self, Mesh, Material=None):

        # Create intermediate mesh
        MaterialIndex = Mesh.materials.values().index(Material) if Material else 0
        temp = MeshExportObject.IntermediateMesh()
        temp.build(Mesh, MaterialIndex,
            self.Config.ExportNormals,
            self.Config.ExportUVCoordinates,
            self.Config.ExportVertexColors,
            self.Config.ExportTangents)

        # No vertices to export for this material
        if not temp.Vertices:
            return False

        self.Exporter.File.Write("<mesh name='{}' size='{}'>\n".format(self.name, len(temp.Vertices) ))
        self.Exporter.File.Indent()

        # Write material name
        if Material:
            self.Exporter.File.Write("<material name='{}'/>\n".format(Material.name))

        # Parent bone
        if self.BlenderObject.parent and self.BlenderObject.parent_type == 'BONE':
            self.Exporter.File.Write("<parent bone='{}'/>\n".format(self.BlenderObject.parent_bone))


        # Write vertices
        self.Exporter.Log("Writing vertices...");
        self.Exporter.File.Write("<vertices>\n")
        self.Exporter.File.Indent()
        for vertex in temp.Vertices:
            self.Exporter.File.Write("{:f} {:f} {:f}\n".format( vertex.Position[0], vertex.Position[1], vertex.Position[2]))
        self.Exporter.File.Unindent()
        self.Exporter.File.Write("</vertices>\n")

        # Write Normals
        if self.Config.ExportNormals:
            self.Exporter.Log("Writing normals...");
            self.Exporter.File.Write("<normals>\n")
            self.Exporter.File.Indent()
            for vertex in temp.Vertices:
                self.Exporter.File.Write("{:f} {:f} {:f}\n".format( vertex.Normal[0], vertex.Normal[1], vertex.Normal[2]))
            self.Exporter.File.Unindent()
            self.Exporter.File.Write("</normals>\n")

        # Write Tangents
        if self.Config.ExportTangents and temp.Vertices[0].Tangent:
            self.Exporter.Log("Writing tangents...");
            self.Exporter.File.Write("<tangents>\n")
            self.Exporter.File.Indent()
            for vertex in temp.Vertices:
                self.Exporter.File.Write("{:f} {:f} {:f}\n".format( vertex.Tangent[0], vertex.Tangent[1], vertex.Tangent[2]))
            self.Exporter.File.Unindent()
            self.Exporter.File.Write("</tangents>\n")


        # Write Colours
        if self.Config.ExportVertexColors and temp.Vertices[0].Colour:
            self.Exporter.Log("Writing vertex colours...");
            self.Exporter.File.Write("<colours channels='3'>\n")
            self.Exporter.File.Indent()
            for vertex in temp.Vertices:
                self.Exporter.File.Write("{:f} {:f} {:f}\n".format( vertex.Colour[0], vertex.Colour[1], vertex.Colour[2]))
            self.Exporter.File.Unindent()
            self.Exporter.File.Write("</colours>\n")

        # Write UVs (Flip V)
        if self.Config.ExportUVCoordinates and temp.Vertices[0].UV:
            self.Exporter.Log("Writing texture coordinates...");
            self.Exporter.File.Write("<texcoords>\n")
            self.Exporter.File.Indent()
            for vertex in temp.Vertices:
                self.Exporter.File.Write("{:f} {:f}\n".format( vertex.UV[0], 1.0 - vertex.UV[1]))
            self.Exporter.File.Unindent()
            self.Exporter.File.Write("</texcoords>\n")


        # Write polygons
        self.Exporter.Log("Writing polygons...");
        self.Exporter.File.Write("<polygons size='{}'>\n".format( len(temp.Indices) ))
        self.Exporter.File.Indent()
        for face in temp.Indices:
            if len(face) != 3: raise ValueError("Polygons must be triangles")
            self.Exporter.File.Write("{} {} {}\n".format( face[0], face[1], face[2]))
        self.Exporter.File.Unindent()
        self.Exporter.File.Write("</polygons>\n")

        # Skins
        if self.Config.ExportSkinWeights:
            #self._WriteSkins(Mesh, temp)
            self._WriteSkins2(Mesh, temp)

        self.Exporter.File.Unindent()
        self.Exporter.File.Write("</mesh>\n")
        return True


    class Skin:
        def __init__(self, name):
            self.indices = []
            self.weights = []
            self.name = name

        def add(self, index, weight):
            self.indices.append(index)
            self.weights.append(weight)


    def _WriteSkins(self, Mesh, Temp):

        # Get armatures
        ArmatureList = [m for m in self.BlenderObject.modifiers if m.type == 'ARMATURE' and m.show_viewport]
        if not ArmatureList:
            return

        for obj in [m.object for m in ArmatureList]:
            # Determine group names
            VertexGroupNames = [Group.name for Group in self.BlenderObject.vertex_groups]
            PoseBoneNames = set([Bone.name for Bone in obj.pose.bones])

            Skins = []
            for Name in VertexGroupNames:
                Skins.append( MeshExportObject.Skin(Name) if Name in PoseBoneNames else None )
            maxWeights = 0

            # Add weights and indices to skins
            for Index, Vertex in enumerate(Mesh.vertices):
                count = 0
                for Group in Vertex.groups:
                    if Skins[Group.group] and Group.weight > 0:
                        Skins[Group.group].add(Index, Group.weight)
                        count += 1
                        if count > maxWeights:
                            maxWeights = count

            #Write skins
            for Skin in Skins:
                if Skin:
                    self.Exporter.File.Write("<skin name='{}' size='{}'>\n".format(Skin.name, len(Skin.indices)))
                    self.Exporter.File.Indent()
                    self.Exporter.File.Write("<indices>".format(Skin.name))
                    for i in Skin.indices:
                        self.Exporter.File.Write("{} ".format(i), False)
                    self.Exporter.File.Write("</indices>\n", False)
                    self.Exporter.File.Write("<weights>".format(Skin.name))
                    for w in Skin.weights:
                        self.Exporter.File.Write("{:f} ".format(w), False)
                    self.Exporter.File.Write("</weights>\n", False)
                    self.Exporter.File.Unindent()
                    self.Exporter.File.Write("</skin>\n")


    # Alternative format - this is closer to the destination format so should be faster to load
    def _WriteSkins2(self, Mesh, Temp):
        # Get armatures
        ArmatureList = [m for m in self.BlenderObject.modifiers if m.type == 'ARMATURE' and m.show_viewport]
        if not ArmatureList:
            return

        Groups = self.BlenderObject.vertex_groups
        GroupMap = [-1] * len(Groups)
        GroupNames = []
        index = 0

        # Compile bone validation lookup
        HasAssiciatedBone = [False] * len(Groups)
        for obj in [m.object for m in ArmatureList]:
            for Bone in obj.pose.bones:
                for Index, Group in enumerate(Groups):
                    if Group.name == Bone.name:
                        HasAssiciatedBone[Index] = True


        # Collect weight info
        maxWeights = 0
        Indices = [[] for x in range(0, len(Temp.Vertices))]
        Weights = [[] for x in range(0, len(Temp.Vertices))]
        for Index, Vertex in enumerate(Mesh.vertices):
            if Temp.Map[Index]:
                for Group in Vertex.groups:
                    if Group.weight != 0.0 and HasAssiciatedBone[Group.group]:
                        # Assign group a new index if new
                        if GroupMap[Group.group] < 0:
                            GroupMap[Group.group] = index;
                            GroupNames.append( Groups[Group.group].name );
                            index += 1

                        for Vx in Temp.Map[Index]:
                            Indices[Vx].append( GroupMap[Group.group] )
                            Weights[Vx].append( Group.weight )

                maxWeights = max(maxWeights, len( Weights[ Temp.Map[Index][0] ] ))

        # Format data
        if maxWeights > 4: print("Warning: Mesh has more than 4 weights on a vertex")
        maxWeights = min(maxWeights, 4)
        for Index in range(0, len(Indices)):
            Indices[Index] = [ x for (y,x) in sorted(zip(Weights[Index], Indices[Index]), reverse=True) ]
            Weights[Index].sort(reverse=True)
            # Pad to length
            Len = len(Indices[Index])
            Indices[Index] += [0] * (maxWeights - Len)
            Weights[Index] += [0.0] * (maxWeights - Len)
            # Truncate to length
            del Indices[Index][maxWeights:]
            del Weights[Index][maxWeights:]
            # Normalise
            total = sum(Weights[Index])
            if total > 0:
                Weights[Index] = [ x / total for x in Weights[Index] ]
            else:
                Weights[Index][0] = 1.0
                print ("Warning: Vertex has no weights assigned")


        # Write weights
        self.Exporter.File.Write("<skin size='{}' weightspervertex='{}'>\n".format(index, maxWeights))
        self.Exporter.File.Indent()
        for Skin in GroupNames:
            self.Exporter.File.Write("<group name='{}'/>\n".format( Skin ))

        self.Exporter.File.Write("<indices>")
        for Vx in Indices:
            for I in Vx:
                self.Exporter.File.Write('{} '.format(I), False)
        self.Exporter.File.Write("</indices>\n", False)

        self.Exporter.File.Write("<weights>")
        for Vx in Weights:
            for W in Vx:
                self.Exporter.File.Write('{:f} '.format(W), False)
        self.Exporter.File.Write("</weights>\n", False)

        self.Exporter.File.Unindent()
        self.Exporter.File.Write("</skin>\n")


# Armature object implementation of ExportObject ===============================================================
class ArmatureExportObject(ExportObject):
    def __init__(self, Config, Exporter, BlenderObject):
        ExportObject.__init__(self, Config, Exporter, BlenderObject)

    def __repr__(self):
        return "[ArmatureExportObject: {}]".format(self.name)

    # "Public" Interface

    def Write(self):
        if self.Config.ExportArmatureBones:
            Armature = self.BlenderObject.data
            RootBones = [Bone for Bone in Armature.bones if Bone.parent is None]
            self.Exporter.Log("Writing skeleton...")
            self.Exporter.File.Write("<skeleton name='{}' size='{}'>\n".format(self.name, len(Armature.bones)))
            self.Exporter.File.Indent()
            self.WriteBones(RootBones)
            self.Exporter.File.Unindent()
            self.Exporter.File.Write("</skeleton>\n");
            self.Exporter.Log("Done")

    # "Private" Methods

    def WriteBones(self, Bones):
        for Bone in Bones:
            BoneMatrix = Matrix()

            rot = Matrix.Rotation(radians(-90), 4, 'X') # Rotate all bones so direction = Z

            if Bone.parent:
                BoneMatrix = (Bone.parent.matrix_local * rot).inverted()
                BoneMatrix *= Bone.matrix_local * rot
            else:
                BoneMatrix = rot * Bone.matrix_local * rot

            self.Exporter.File.Write("<bone name='{}' length='{:f}'>\n".format(Bone.name, Bone.length))
            self.Exporter.File.Indent()

            Util.WriteMatrix(self.Exporter.File, BoneMatrix)
            self.WriteBones(Bone.children);

            self.Exporter.File.Unindent()
            self.Exporter.File.Write("</bone>\n")




# Container for animation data =================================================================================
class AnimationKeySet:
    def __init__(self, Target):
        self.Target = Target
        self.RotationKeys = []
        self.ScaleKeys = []
        self.PositionKeys = []

    def optimise(self):
        def cmpVec(a, b, e=0.00001):
            return abs(a[0]-b[0]) < e and abs(a[1]-b[1]) < e and abs(a[2]-b[2]) < e

        def cmpQuat(a, b, e=0.00001):
            return abs(a[0]-b[0]) < e and abs(a[1]-b[1]) < e and abs(a[2]-b[2]) < e and abs(a[3]-b[3]) < e

        import mathutils
        AnimationKeySet.optimiseKeys(self.RotationKeys, mathutils.Quaternion((1,0,0,0)), cmpQuat)
        AnimationKeySet.optimiseKeys(self.PositionKeys, mathutils.Vector((0,0,0)), cmpVec)
        AnimationKeySet.optimiseKeys(self.ScaleKeys,    mathutils.Vector((1,1,1)), cmpVec)

    def optimiseKeys(Keys, Zero, Cmp):
        First = Keys[0][1]
        for K in Keys:
            if not Cmp(K[1], First):
                return
        if Cmp(First, Zero):
            del Keys[0:]
        else:
            del Keys[1:]

    def hasKeys(self):
        return self.RotationKeys or self.ScaleKeys or self.PositionKeys

# Creates a list of Animation objects based on the animation needs of the
# ExportObject passed to it
class AnimationGenerator: # Base class, do not use
    def __init__(self, Config, Name, ExportObject):
        self.Config = Config
        self.name = Name
        self.ExportObject = ExportObject
        self.KeySets = []
        self.length = 0


# Creates one Animation object that contains the rotation, scale, and position
# of the ExportObject
class GenericAnimationGenerator(AnimationGenerator):
    def __init__(self, Config, Name, ExportObject):
        AnimationGenerator.__init__(self, Config, Name, ExportObject)
        self._GenerateKeys()

    def _GenerateKeys(self):
        Scene = bpy.context.scene # Convenience alias
        BlenderCurrentFrame = Scene.frame_current
        CurrentAnimation = AnimationKeySet(self.ExportObject.name)
        BlenderObject = self.ExportObject.BlenderObject
        start, end = BlenderObject.animation_data.action.frame_range
        self.length = int(end - start) + 1

        for Frame in range(self.length):
            Scene.frame_set(Frame + start)

            Rotation = BlenderObject.rotation_euler.to_quaternion()
            Scale = BlenderObject.matrix_local.to_scale()
            Position = BlenderObject.matrix_local.to_translation()

            CurrentAnimation.RotationKeys.append((Frame, Rotation))
            CurrentAnimation.ScaleKeys.append((Frame, Scale))
            CurrentAnimation.PositionKeys.append((Frame, Position))

        CurrentAnimation.optimise()
        if CurrentAnimation.hasKeys():
            self.KeySets.append(CurrentAnimation)

        Scene.frame_set(BlenderCurrentFrame)

# Essentially a bunch of GenericAnimationGenerators lumped into one.
class GroupAnimationGenerator(AnimationGenerator):
    def __init__(self, Config, Name, ExportObjects):
        AnimationGenerator.__init__(self, Config, Name, None)
        self.ExportObjects = ExportObjects
        self._GenerateKeys()

    def _GenerateKeys(self):
        for Object in self.ExportObjects:
            if Object.BlenderObject.type == 'ARMATURE':
                TemporaryGenerator = ArmatureAnimationGenerator(self.Config, None, Object)
                self.KeySets += TemporaryGenerator.KeySets
            else:
                TemporaryGenerator = GenericAnimationGenerator(self.Config, None, Object)
                self.KeySets += TemporaryGenerator.Animations


# Creates an Animation object for the ArmatureExportObject it gets passed and
# an Animation object for each bone in the armature (if options allow)
class ArmatureAnimationGenerator(GenericAnimationGenerator):
    def __init__(self, Config, Name, ArmatureExportObject):
        GenericAnimationGenerator.__init__(self, Config, Name, ArmatureExportObject)
        self._GenerateBoneKeys()

    def _GenerateBoneKeys(self):
        from itertools import zip_longest as zip

        Scene = bpy.context.scene # Convenience alias
        BlenderCurrentFrame = Scene.frame_current
        ArmatureObject = self.ExportObject.BlenderObject
        ArmatureName = self.ExportObject.name
        Action = ArmatureObject.animation_data.action

        # Rotation fix
        rot = Matrix.Rotation(radians(-90), 4, 'X') # Rotate all bones so direction = Z
        qrot = rot.to_quaternion();

        # create keysets
        AnimationData = []
        for bone in ArmatureObject.pose.bones:
            if bone.name in Action.groups.keys():
                AnimationData.append( (bone, AnimationKeySet(bone.name)) )


        # Extract keyframe data
        start, end = Action.frame_range
        self.length = int(end - start) + 1
        for Frame in range(self.length):
            Scene.frame_set(Frame + start)
            for Bone, Keys in AnimationData:

                PoseMatrix = Matrix()
                if Bone.parent:
                    PoseMatrix = (Bone.parent.matrix * rot).inverted()
                    PoseMatrix *= Bone.matrix * rot
                else:
                    PoseMatrix = rot * Bone.matrix * rot

                Rotation = qrot.inverted() * Bone.rotation_quaternion * qrot

                Scale = PoseMatrix.to_scale()
                Position = PoseMatrix.to_translation()

                Keys.RotationKeys.append((Frame, Rotation))
                Keys.PositionKeys.append((Frame, Position))
                Keys.ScaleKeys.append((Frame, Scale))

        # Delete unnessesary keys
        for _,Keys in AnimationData:
            Keys.optimise()
            if Keys.hasKeys():
                self.KeySets.append(Keys)

        Scene.frame_set(BlenderCurrentFrame)


# Writes all animation data to file. ===========================================================================
class AnimationWriter:
    def __init__(self, Config, Exporter, AnimationGenerators):
        self.Config = Config
        self.Exporter = Exporter
        self.Animations = AnimationGenerators

    def WriteAnimations(self):
        FrameRate = int(bpy.context.scene.render.fps / bpy.context.scene.render.fps_base)

        for Animation in self.Animations:
            self.Exporter.Log("Writing animation {}".format(Animation.name))
            self.Exporter.File.Write("<animation name='{}' frames='{}' rate='{}'>\n".format(Animation.name, Animation.length, FrameRate))
            self.Exporter.File.Indent()

            # Write each keyset
            for Keys in Animation.KeySets:
                self.Exporter.File.Write("<keyset target='{}'>\n".format(Keys.Target))
                self.Exporter.File.Indent()

                # Write animation keys
                if Keys.RotationKeys:
                    self.Exporter.File.Write("<rotation size='{}'>\n".format( len(Keys.RotationKeys)))
                    self.Exporter.File.Indent()
                    for Frame, Value in Keys.RotationKeys:
                        self.Exporter.File.Write("<key frame='{}' value='{:f} {:f} {:f} {:f}'/>\n".format(Frame, Value[0], Value[1], Value[2], Value[3]))
                    self.Exporter.File.Unindent()
                    self.Exporter.File.Write("</rotation>\n");


                if Keys.PositionKeys:
                    self.Exporter.File.Write("<position size='{}'>\n".format( len(Keys.PositionKeys)))
                    self.Exporter.File.Indent()
                    for Frame, Value in Keys.PositionKeys:
                        self.Exporter.File.Write("<key frame='{}' value='{:f} {:f} {:f}'/>\n".format(Frame, Value[0], Value[1], Value[2]))
                    self.Exporter.File.Unindent()
                    self.Exporter.File.Write("</position>\n");



                if Keys.ScaleKeys:
                    self.Exporter.File.Write("<scale size='{}'>\n".format( len(Keys.ScaleKeys)))
                    self.Exporter.File.Indent()
                    for Frame, Value in Keys.ScaleKeys:
                        self.Exporter.File.Write("<key frame='{}' value='{:f} {:f} {:f}'/>\n".format(Frame, Value[0], Value[1], Value[2]))
                    self.Exporter.File.Unindent()
                    self.Exporter.File.Write("</scale>\n");


                self.Exporter.File.Unindent()
                self.Exporter.File.Write("</keyset>\n")

            self.Exporter.File.Unindent()
            self.Exporter.File.Write("</animation>\n")



# Interface to the file.  Supports automatic whitespace indenting. =============================================
class File:
    def __init__(self, FilePath):
        self.FilePath = FilePath
        self.File = None
        self.__Whitespace = 0

    def Open(self):
        if not self.File:
            self.File = open(self.FilePath, 'w')

    def Close(self):
        self.File.close()
        self.File = None

    def Write(self, String, Indent=True):
        if Indent:
            # Escape any formatting braces
            String = String.replace("{", "{{")
            String = String.replace("}", "}}")
            self.File.write(("{}" + String).format("  " * self.__Whitespace))
        else:
            self.File.write(String)

    def Indent(self, Levels=1):
        self.__Whitespace += Levels

    def Unindent(self, Levels=1):
        self.__Whitespace -= Levels
        if self.__Whitespace < 0:
            self.__Whitespace = 0


# Static utilities
class Util:

    @staticmethod
    def WriteMatrix(File, Matrix):
        File.Write("<matrix>");
        File.Write("{:f} {:f} {:f} {:f} ".format(Matrix[0][0], Matrix[1][0], Matrix[2][0], Matrix[3][0]), False)
        File.Write("{:f} {:f} {:f} {:f} ".format(Matrix[0][1], Matrix[1][1], Matrix[2][1], Matrix[3][1]), False)
        File.Write("{:f} {:f} {:f} {:f} ".format(Matrix[0][2], Matrix[1][2], Matrix[2][2], Matrix[3][2]), False)
        File.Write("{:f} {:f} {:f} {:f}".format(Matrix[0][3], Matrix[1][3], Matrix[2][3], Matrix[3][3]), False)
        File.Write("</matrix>\n", False);

    # Used on lists of blender objects and lists of ExportObjects, both of
    # which have a name field
    @staticmethod
    def SortByNameField(List):
        def SortKey(x):
            return x.name
        return sorted(List, key=SortKey)

