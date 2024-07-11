## Complete rewrite of the bm exporter ##

import os
import bpy
import bmesh
import mathutils
import bpy_extras.io_utils

from xml.dom.minidom import Document
from mathutils import Matrix
from math import radians
#from progress_report import ProgressReport, ProgressReportSubstep


# -------------------------------------------------------------------------- #

def create_document(root):
    xml = Document()
    xml.appendChild( xml.createElement(root) )
    return xml

def append_element(parent, name):
    node = parent.ownerDocument.createElement(name)
    parent.appendChild(node)
    return node

def append_text(parent, name, text):
    node = append_element(parent, name)
    set_text(node, text)

def set_text(parent, text):
    node = parent.ownerDocument.createTextNode(text)
    parent.appendChild(node)

def format_num(value):
    return str( round(value,6) )

def modified(obj, config):
    return config.apply_modifiers and obj.type=='MESH' and len(obj.modifiers) and obj.data.users > 1


# -------------------------------------------------------------------------- #

def init_progress(context, total):
    global current_progress
    current_progress = 0
    context.window_manager.progress_begin(0, total)

def end_progress(context):
    context.window_manager.progress_end()

def advance_progress(context):
    global current_progress
    current_progress += 1
    context.window_manager.progress_update(current_progress)
    


# -------------------------------------------------------------------------- #

class Vertex(object):
    def __init__(self, co):
        self.pos = co;
        self.nrm = (0,0,0)
        self.tex = (0,0)
        self.col = (1,1,1,1)
        self.tan = (0,0,0,1)
        self.wgt = None
    def __hash__(self):
        return hash( (self.pos, self.nrm, self.tex, self.col, self.tan) )
    def __eq__(self, other):
        return self.pos == other.pos and self.nrm == other.nrm and self.tex == other.tex \
                and self.col == other.col and self.tan == other.tan

class Mesh:
    def __init__(self, material, faces, vertices):
        self.vertices = vertices    # Vertex data
        self.faces = faces          # Face data
        self.weights = 0            # Weights per vertex
        self.groups = {}            # Weight group map
        self.material = material    # material data
        self.has_normal = False
        self.has_tangent = False
        self.has_colour = False
        self.has_uv = False

class KeySet:
    def __init__(self, target, pos, rot, scl):
        self.target = target
        self.rotation = [] if rot else None
        self.position = [] if pos else None
        self.scale    = [] if scl else None

# -------------------------------------------------------------------------- #

def triangulate(mesh):
    import bmesh
    bm = bmesh.new()
    bm.from_mesh(mesh)
    bmesh.ops.triangulate(bm, faces=bm.faces)
    bm.to_mesh(mesh)
    bm.free()

def make_colour(col, alpha):
    return (col[0], col[1], col[2], alpha[0])

def construct_mesh(obj, config):
    if config.apply_modifiers:
        armatureModifiers = [m for m in obj.modifiers if m.type == 'ARMATURE' and m.show_viewport]
        for m in armatureModifiers: m.show_viewport = False
        obj = obj.evaluated_get(bpy.context.evaluated_depsgraph_get())
        for m in armatureModifiers: m.show_viewport = True
    m = obj.to_mesh()

    # object transform - this is for up axis / coordinate system conversion
    #trans = config.global_matrix;
    trans = Matrix.Rotation(radians(-90), 4, 'X')
    if config.apply_transform:
        trans = trans @ obj.matrix
    m.transform(trans)
    if trans.determinant() < 0.0:
        m.flip_normals()
    meshes = construct_export_meshes(m, config)
    obj.to_mesh_clear()
    return meshes

def construct_export_meshes(m, config):

    needTriangulating = False
    for f in m.polygons:
        if len(f.vertices) > 3:
            needTriangulating = True
            break

    # Triangulate
    if needTriangulating:
        triangulate(m);

    # Additional per-vertex data
    uv  = m.uv_layers.active.data[:] if config.export_uv and len(m.uv_layers) > 0 else None
    col = m.vertex_colors.active.data[:] if config.export_colour and len(m.vertex_colors) > 0 else None
    alpha = None
    white = (1,1,1,1)
    if config.export_colour:
        for c in m.vertex_colors.items():
            if not c[1].active and c[0].lower() == 'alpha':
                alpha = col[1].data


    # Update normals and tangents
    signedtangents = False
    if uv and config.export_tangents:
        m.calc_tangents()
        for loop in m.loops:
            if loop.bitangent_sign < 0:
                signedtangents = True
                break
    #elif config.export_normals: # removed in Blender 4.1
    #    m.calc_normals_split()


    # sort faces by material index for separation into sub-meshes
    meshFaces = m.polygons[:]
    meshFaces.sort(key = lambda a: (a.material_index))

    # Output mesh
    lastMaterial = -2
    mesh = None
    result = []

    for face in meshFaces:
        if len(face.vertices) != 3: raise ValueError("Polygon not a triangle")

        # Create mesh object
        if face.material_index != lastMaterial:
            material = m.materials[ face.material_index ] if m.materials else None
            mesh = Mesh( material, [], [] )
            mesh.has_uv = uv;
            mesh.has_normal = config.export_normals
            mesh.has_colour = col
            mesh.has_tangent = uv and config.export_tangents
            mesh.weights = 0
            result.append(mesh)
            vmap = {}
            lastMaterial = face.material_index


        # get the three vertices
        for i in range(3):
            loop = face.loop_indices[i]
            index = face.vertices[i]
            v = Vertex( m.vertices[index].co.to_tuple() )
            if uv: v.tex = (uv[ loop ].uv.x, 1-uv[ loop ].uv.y)
            if config.export_normals: v.nrm = m.loops[ loop ].normal.to_tuple()
            if uv and config.export_tangents:
                v.tan = m.loops[loop].tangent.to_tuple()
                if signedtangents: v.tan += (m.loops[loop].bitangent_sign,)
            if col: v.col = make_colour( col[loop].color, alpha[loop].color if alpha else white)

            # vertex groups
            if m.vertices[index].groups and config.export_skins:
                v.wgt = []
                for group in m.vertices[index].groups:
                    if group.weight > 0.001:
                        g = obj.vertex_groups[ group.group ]
                        index = mesh.groups.setdefault(g.name, len(mesh.groups))
                        v.wgt.append( (index, group.weight) )
                if config.normalise_weights:
                    process_weights(v, 4, True)
                mesh.weights = max(mesh.weights, len(v.wgt))

            # add vertex if new
            vIndex = vmap.get(v)
            if vIndex == None:
                vIndex = len(mesh.vertices)
                mesh.vertices.append(v)
                vmap[v] = vIndex

            # Add vertex to face
            mesh.faces.append(vIndex)

    return result

def process_weights(v, limit, normalise):
    if limit < len(v.wgt):
        v.wgt = sorted(v.wgt, key=lambda w:w[1], reverse=True)[:limit]

    if normalise:
        total = 0
        for g in v.wgt: total += g[1]
        if total != 1.0 and total != 0:
            for i in range(len(v.wgt)): v.wgt[i] = (v.wgt[i][0], v.wgt[i][1]/total)

# -------------------------------------------------------------------------- #

def export_merged_meshes(obj, exportList, config, xml):
    bm = bmesh.new()
    local = obj.matrix_world.inverted()
    for object in exportList:
        if object.type == 'MESH':
            o = object
            if config.apply_modifiers:
                armatureModifiers = [m for m in o.modifiers if m.type == 'ARMATURE' and m.show_viewport]
                for m in armatureModifiers: m.show_viewport = False
                o = o.evaluated_get(bpy.context.evaluated_depsgraph_get())
                for m in armatureModifiers: m.show_viewport = True

            trans = Matrix.Rotation(radians(-90), 4, 'X') @ local @ o.matrix_world
            m = o.to_mesh()
            m.transform(trans)
            if trans.determinant() < 0.0:
                m.flip_normals()
            bm.from_mesh(m)
            o.to_mesh_clear()
            # FIXME: Materials on merged mesh are missing
        
    mesh = bpy.data.meshes.new('TempCombinedMesh')
    bm.to_mesh(mesh)
    mesh.update()
    bm.free()

    meshes = construct_export_meshes(mesh, config)
    write_meshes(obj, config, xml, meshes)
    bpy.data.meshes.remove(mesh)


def export_mesh(obj, config, xml):
    meshes = construct_mesh(obj, config)
    write_meshes(obj, config, xml, meshes)

def write_meshes(obj, config, xml, meshes):
    name = obj.name if modified(obj,config) else obj.data.name
    for m in meshes:
        mesh = append_element(xml.firstChild, "mesh")
        mesh.setAttribute("name", name)                 #NOTE: duplicate name if mesh is split by material
        mesh.setAttribute("size", str(len(m.vertices)))

        export_custom_properties(mesh, obj.data, "properties")

        if m.material:
            mat = append_element(mesh, "material")
            mat.setAttribute("name", m.material.name)

        verts = append_element(mesh, "vertices")
        vertData = [' '.join([format_num(e) for e in v.pos]) for v in m.vertices]
        set_text(verts, '  '.join(vertData))

        if m.has_normal:
            node = append_element(mesh, "normals")
            vertData = [' '.join([format_num(e) for e in v.nrm]) for v in m.vertices]
            set_text(node, '  '.join(vertData))

        if m.has_tangent:
            node = append_element(mesh, "tangents")
            if len(m.vertices[0].tan)==4: node.setAttribute("elements", "4");
            vertData = [' '.join([format_num(e) for e in v.tan]) for v in m.vertices]
            set_text(node, '  '.join(vertData))

        if m.has_colour:
            node = append_element(mesh, "colours")
            vertData = [' '.join([format_num(e) for e in v.col]) for v in m.vertices]
            set_text(node, '  '.join(vertData))

        if m.has_uv:
            node = append_element(mesh, "texcoords")
            vertData = [' '.join([format_num(e) for e in v.tex]) for v in m.vertices]
            set_text(node, '  '.join(vertData))

        node = append_element(mesh, "polygons")
        node.setAttribute("size", str(int(len(m.faces)/3)))
        faceData = ' '.join(str(f) for f in m.faces)
        set_text(node, faceData)

        if m.weights > 0:
            node = append_element(mesh, "skin")
            node.setAttribute("size", str(len(m.groups)))
            node.setAttribute("weightspervertex", str(m.weights))
            for g in m.groups:
                group = append_element(node, "group")
                group.setAttribute("name", g)

            # Fix crash if a vertex has no weights in blender
            for v in m.vertices:
                if v.wgt is None: v.wgt = []

            indices = append_element(node, "indices")
            ind = [' '.join( [str(g[0]) for g in v.wgt] + ['0']*(m.weights-len(v.wgt))) for v in m.vertices]
            set_text(indices, ' '.join(ind))

            weights = append_element(node, "weights")
            wgt = [' '.join( [format_num(g[1]) for g in v.wgt] + ['0.0']*(m.weights-len(v.wgt))) for v in m.vertices]
            set_text(weights, ' '.join(wgt))


# -------------------------------------------------------------------------- #

def export_skeleton(skeleton, xml):
    node = append_element(xml.firstChild, "skeleton")
    node.setAttribute("name", skeleton.name)
    node.setAttribute("size", str(len(skeleton.data.bones)))

    rootBones = [b for b in skeleton.data.bones if b.parent is None]
    write_bones(rootBones, node)

def write_bones(bones, parent):
    for bone in bones:
        matrix = Matrix()
        rot = Matrix.Rotation(radians(-90), 4, 'X') # Up axis fix
        if bone.parent: matrix = (bone.parent.matrix_local @ rot).inverted() @ bone.matrix_local @ rot
        else: matrix = rot @ bone.matrix_local @ rot

        node = append_element(parent, "bone")
        node.setAttribute("name", bone.name)
        node.setAttribute("length", format_num(bone.length))

        matrix.transpose()
        m = [' '.join( format_num(v) for v in vec) for vec in matrix ]
        append_text(node, "matrix", ' '.join(m))

        # Child bones
        write_bones(bone.children, node)

# -------------------------------------------------------------------------- #

def export_animations(context, config, skeleton, xml):
    if skeleton.animation_data:
        actions = []
        if skeleton.animation_data.action:
            actions.append( (skeleton.animation_data.action, "Action") )

        # Modifier types to disable
        disableModifiers = [ 'TRIANGULATE', 'SUBSURF', 'DISPLACE', 'DECIMATE' ]

        restore = {}
        if skeleton.animation_data.nla_tracks and (config.export_animations == "LINKED" or config.export_animations == "UNLOCKED"):
            unlocked_only = config.export_animations == "UNLOCKED"
            mode = config.animation_names
            for track in skeleton.animation_data.nla_tracks.values():
                if track.lock and unlocked_only: continue;
                restore[track] = (track.mute, track.is_solo)
                track.mute = True
                track.is_solo = False
                for strip in track.strips.values():
                    if strip.action and strip.action not in actions:
                        name = strip.name if mode == "STRIP" else track.name if mode == "TRACK" else strip.action.name
                        actions.append((strip.action, name))

        if actions:
            active = context.view_layer.objects.active
            frame = context.scene.frame_current
            context.scene.tool_settings.use_keyframe_insert_auto = False # this really messes things up if left on
            last = skeleton.animation_data.action
            context.view_layer.objects.active = skeleton
            bpy.ops.object.mode_set(mode='POSE')

            # Save pose
            transforms = []
            for b in skeleton.pose.bones:
                transforms.append((b, b.location.copy(), b.rotation_quaternion.copy(), b.scale.copy()))

            # Disable any SUBSURF modifiers as they slow things down massively
            modifiers = []
            for o in context.view_layer.objects:
                modifiers += [m for m in o.modifiers if m.show_viewport and m.type in disableModifiers]
            for m in modifiers: m.show_viewport = False


            frameCount = 0;
            for action in actions: frameCount += int(action[0].frame_range[1] - action[0].frame_range[0]) + 1
            init_progress(context, frameCount)

            # Export actions
            for action in actions:
                export_action(context, skeleton, action[0], action[1], xml)

            # Restore
            end_progress(context);
            for m in modifiers: m.show_viewport = True
            context.view_layer.objects.active = active
            skeleton.animation_data.action = last
            for track, data in restore.items():
                track.mute = data[0]
                track.is_solo = data[1]

            # restore pose
            context.scene.frame_set(frame)
            for b in transforms:
                b[0].location = b[1];
                b[0].rotation_quaternion = b[2]
                b[0].scale = b[3]

            # Restore disabled modifiers
            for m in modifiers: m.show_viewport = True


def same(a,b): return abs(a-b)<0.00001

def export_action(context, skeleton, action, name, xml):
    print("Context", context.mode, context.active_object.name);
    skeleton.animation_data.action = None
    bpy.ops.pose.user_transforms_clear(False)
    skeleton.animation_data.action = action

    mrot = Matrix.Rotation(radians(-90), 4, 'X') # Up axis fix
    qrot = mrot.to_quaternion()
    print(action.name)

    bpy.ops.pose.select_all(action='SELECT')
    bpy.ops.pose.transforms_clear()

    data = []
    for bone in skeleton.pose.bones:
        if bone.name in action.groups.keys():
            group = action.groups[bone.name]
            hasLocation = any('location' in channel.data_path for channel in group.channels)
            hasRotation = any('rotation' in channel.data_path for channel in group.channels)
            hasScale    = any('scale' in channel.data_path for channel in group.channels)
            data.append( (bone, KeySet(bone.name, hasLocation, hasRotation, hasScale)) )

    #  Get key data
    start, end = action.frame_range
    length = int(end - start) + 1
    start = int(start)
    for frame in range(length):
        context.scene.frame_set(start + frame)
        advance_progress(context);
        for bone, keys in data:
            if keys.position is not None:
                pos = bone.location.copy() @ mrot
                keys.position.append( (frame, pos) )
            if keys.rotation is not None:
                rot = qrot.inverted() @ bone.rotation_quaternion @ qrot
                keys.rotation.append( (frame, rot.normalized()) )
            if keys.scale is not None:
                keys.scale.append( (frame, bone.scale.xzy) )

    # Optimise
    for _,keys in data:
        optimise_keys(keys.position, (0,0,0),   lambda a,b: same(a[0],b[0]) and same(a[1],b[1]) and same(a[2],b[2]))
        optimise_keys(keys.rotation, (1,0,0,0), lambda a,b: same(a[0],b[0]) and same(a[1],b[1]) and same(a[2],b[2]) and same(a[3],b[3]))
        optimise_keys(keys.scale,    (1,1,1),   lambda a,b: same(a[0],b[0]) and same(a[1],b[1]) and same(a[2],b[2]))

    # Write xml
    anim = append_element(xml.firstChild, "animation")
    anim.setAttribute("name", name)
    anim.setAttribute("frames", str(length))
    anim.setAttribute("rate", str(context.scene.render.fps))
    export_custom_properties(anim, action, "properties")
    for _,keys in data:
        if keys.rotation or keys.position or keys.scale:
            keyset = append_element(anim, "keyset")
            keyset.setAttribute("target", keys.target)
            if keys.rotation: write_keyframes(keyset, "rotation", keys.rotation)
            if keys.position: write_keyframes(keyset, "position", keys.position)
            if keys.scale: write_keyframes(keyset, "scale", keys.scale)

def optimise_keys(keys, identity, compare):
    if not keys: return
    first = keys[0][1]
    for k in keys:
        if not compare(k[1], first):
            return
    if compare(first, identity): del keys[0:]
    else: del keys[1:]

def write_keyframes(keyset, name, data):
    node = append_element(keyset, name)
    node.setAttribute("keys", str(len(data)))
    for frame,values in data:
        key = append_element(node, "key")
        key.setAttribute("frame", str(frame))
        key.setAttribute("value", ' '.join( str( round(v,6) ) for v in values ));

# -------------------------------------------------------------------------- #

def export_scene(objects, config, xml):

    # Figure out heirachy
    children = { None:[] }
    for obj in objects: children[obj] = []
    for obj in objects:
        p = obj.parent
        for c in obj.constraints:
            if c.type == 'CHILD_OF' and c.enabled and c.is_valid: p = c.target
        while(True):
            if p in children:
                children[p].append(obj)
                break
            p = p.parent

    print(children);

    skipRootTransform = config.clear_layout_root and len(children[None]) == 1

    first = next(iter(objects)).users_collection
    singleCollection = all(v.users_collection == first for v in objects)
    offset = first[0].instance_offset if config.clear_layout_root and singleCollection else Vector((0,0,0))

    # Write xml
    scene = append_element(xml.firstChild, "layout")
    stack = [(None,0, scene)]
    while len(stack)>0:
        print(stack)
        a = stack[-1]
        items = children[a[0]]
        obj = items[a[1]]
        node = write_object(a[2], obj, config, offset, skipRootTransform and a[0] == None)
        if a[1] + 1 >= len(items): stack.pop()
        else: stack[-1] = (a[0], a[1]+1, a[2])

        if children[obj]:
            stack.append( (obj, 0, node) )



def write_object(node, obj, config, offset, skipTransform):
    print("object", obj.name)

    if obj.rotation_mode == 'QUATERNION': r = obj.rotation_quaternion
    elif obj.rotation_mode == 'AXIS_ANGLE': r = Quaternion(obj.rotation_axis_angle[1:4], obj.rotation_axis_angle[0])
    else: r = obj.rotation_euler.to_quaternion()
    pos = obj.location - offset

    # Parent bone stuff
    bone = None
    inverse = obj.matrix_parent_inverse if obj.parent else None
    if obj.parent_bone: bone = obj.parent_bone
    else:
        for c in obj.constraints:
            if c.type == 'CHILD_OF' and c.is_valid and c.enabled:
                bone = c.subtarget;
                inverse = c.inverse_matrix
                break

    if inverse:
        pos = inverse @ pos
        r = inverse.to_quaternion() @ r
        
        if bone: 
            r = mathutils.Quaternion((0,1,0,0)) @ r
            pos.y *= -1
            pos.z *= -1

            # Parenting to a bone puts it at the head point for some reason
            if obj.parent_bone:
                pos.y -= obj.parent.pose.bones[obj.parent_bone].length


    rot = (r.w, r.x, r.z, -r.y)
    pos = (pos.x, pos.z, -pos.y)
    scl = obj.scale.xzy.to_tuple()

    node = append_element(node, "object")
    node.setAttribute("name", obj.name)
    if not skipTransform:
        if pos != (0,0,0):   node.setAttribute("position", ' '.join( format_num(v) for v in pos ))
        if rot != (1,0,0,0): node.setAttribute("orientation", ' '.join( format_num(v) for v in rot ))
        if scl != (1,1,1):   node.setAttribute("scale", ' '.join( format_num(v) for v in scl ))
    if obj.type == 'MESH': node.setAttribute( "mesh", obj.name if modified(obj,config) else obj.data.name)
    if obj.instance_type == 'COLLECTION' and obj.instance_collection: node.setAttribute("instance", obj.instance_collection.name)
    if bone: node.setAttribute("bone", bone)

    if obj.type == 'EMPTY':
        if obj.empty_display_type == 'CUBE': node.setAttribute("shape", "box")
        elif obj.empty_display_type == 'CIRCLE': node.setAttribute("shape", "circle")
        elif obj.empty_display_type == 'SPHERE': node.setAttribute("shape", "sphere")

    if obj.type == 'LIGHT':
        node.setAttribute("light", ' '.join( format_num(v) for v in obj.color[:3] ))

    
    export_custom_properties(node, obj)

    return node


def export_custom_properties(node, obj, name=None):
    if len(obj.keys()) > 0:
        custom = []
        for key in obj.keys():
            if key not in '_RNA_UI':
                custom.append( (key, str(obj[key])) )

        if custom:
            if name: node = append_element(node, name)
            for p in custom: node.setAttribute(p[0], p[1]);


# -------------------------------------------------------------------------- #

def add_heirachy(obj, out):
    out.add(obj)
    for o in obj.children: 
        add_heirachy(o, out)

def export(context, config):
    # What to export
    if config.export_group == "SELECTED":
        exportList = list(context.selected_objects)
    elif config.export_group == "HEIRACHY":
        exportList = set()
        for obj in context.selected_objects:
            add_heirachy(obj, exportList)
    elif config.export_group == "COLLECTION":
        exportList = set()
        collections = set()
        if context.active_object:
            collections = set(context.active_object.users_collection)
        for obj in context.selected_objects:
            collections |= set(obj.users_collection)
        for collection in collections:
            exportList |= set(collection.objects);
    elif config.export_group == "VISIBLE":
        exportList = list(filter(lambda o : not o.hide_viewport, context.view_layer.objects))
    elif config.export_group == "ALL":
        exportList = list(context.view_layer.objects)

    # Linked skeleton
    exportAnimation = config.export_animations != "NONE"
    if config.export_skeletons or exportAnimation:
        armatures = set()
        for obj in exportList:
            if obj.type == 'MESH':
                arm = obj.find_armature()
                if not arm and obj.parent and obj.parent.type == 'ARMATURE': arm = obj.parent
                if not arm:
                    for c in obj.constraints:
                        if c.type == 'CHILD_OF' and c.target and c.target.type == 'ARMATURE': arm = c.target
                if arm: armatures.add(arm)

        for arm in armatures:
            if arm not in exportList:
                if type(exportList) is set: exportList.add(arm)
                else: exportList.append(arm)

    xml = create_document("model")
    xml.firstChild.setAttribute("version", "2.0");

    # Need to not duplicate shared meshes
    exportedMeshes = []

    # Export as a single merged mesh
    if config.merge_meshes and config.export_meshes:
        export_merged_meshes(context.active_object, exportList, config, xml)
        exportedMeshes.append(obj.data.name)

    # Export some stuff
    for obj in exportList:
        if obj.type == 'MESH' and config.export_meshes and not config.merge_meshes:
            if modified(obj,config) or obj.data.name not in exportedMeshes:
                export_mesh(obj, config, xml)
                exportedMeshes.append( obj.data.name )

        elif obj.type == 'ARMATURE' and config.export_skeletons:
            export_skeleton(obj, xml)

    # Animations
    if exportAnimation:
        for obj in exportList:
            if obj.type == 'ARMATURE':
                export_animations(context, config, obj, xml);

    # Layout
    if config.export_layout:
        export_scene(exportList, config, xml)

    # save
    print("Saving to", config.filepath)
    s = xml.toprettyxml('  ')
    f = open(config.filepath, "w")
    f.write( xml.toprettyxml('\t') )
    f.close()

