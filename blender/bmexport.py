## Complete rewrite of the bm exporter ##

import os
import bpy
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

class Vertex(object):
    def __init__(self, co):
        self.pos = co;
        self.nrm = (0,0,0)
        self.tex = (0,0)
        self.col = (1,1,1,1)
        self.tan = (0,0,0)
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
    def __init__(self, target):
        self.target = target
        self.rotation = []
        self.position = []
        self.scale    = []

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
    if config.apply_modifiers: obj = obj.evaluated_get(bpy.context.evaluated_depsgraph_get())
    m = obj.to_mesh()

    # object transform - this is for up axis / coordinate system conversion
    #trans = config.global_matrix;
    trans = Matrix.Rotation(radians(-90), 4, 'X')
    if config.apply_transform:
        trans = trans @ obj.matrix
    m.transform(trans)
    if trans.determinant() < 0.0:
        m.flip_normals()

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
    if uv and config.export_tangents:
        m.calc_tangents()
    elif config.export_normals:
        m.calc_normals_split()


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
            if uv and config.export_tangents: v.tan = m.loops[ loop ].tangent.to_tuple()
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

    obj.to_mesh_clear()
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

def export_mesh(obj, config, xml):
    meshes = construct_mesh(obj, config)
    name = obj.name if modified(obj,config) else obj.data.name

    # write meshes
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
            actions.append( skeleton.animation_data.action )

        if skeleton.animation_data.nla_tracks and config.export_animations == "LINKED":
            for track in skeleton.animation_data.nla_tracks.values():
                track.mute = True
                track.is_solo = False
                for strip in track.strips.values():
                    if strip.action and strip.action not in actions:
                        actions.append(strip.action)

        # Export actions
        context.scene.tool_settings.use_keyframe_insert_auto = False # this really messes things up if left on
        last = skeleton.animation_data.action
        context.view_layer.objects.active = skeleton
        bpy.ops.object.mode_set(mode='POSE')
        for action in actions:
            export_action(context, skeleton, action, xml)
        skeleton.animation_data.action = last

def same(a,b): return abs(a-b)<0.00001

def export_action(context, skeleton, action, xml):
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
            data.append( (bone, KeySet(bone.name)) )

    #  Get key data
    start, end = action.frame_range
    length = int(end - start) + 1
    for frame in range(length):
        context.scene.frame_set(start + frame)
        for bone, keys in data:
            rot = qrot.inverted() @ bone.rotation_quaternion @ qrot
            pos = bone.location.copy();

            if bone.parent: pos = pos @ mrot # Hack ?
            keys.rotation.append( (frame, rot) )
            keys.scale.append( (frame, bone.scale.copy()) )
            keys.position.append( (frame, pos) )

    # Optimise
    for _,keys in data:
        optimise_keys(keys.position, (0,0,0),   lambda a,b: same(a[0],b[0]) and same(a[1],b[1]) and same(a[2],b[2]))
        optimise_keys(keys.rotation, (1,0,0,0), lambda a,b: same(a[0],b[0]) and same(a[1],b[1]) and same(a[2],b[2]) and same(a[3],b[3]))
        optimise_keys(keys.scale,    (1,1,1),   lambda a,b: same(a[0],b[0]) and same(a[1],b[1]) and same(a[2],b[2]))

    # Write xml
    anim = append_element(xml.firstChild, "animation")
    anim.setAttribute("name", action.name)
    anim.setAttribute("frames", str(length))
    anim.setAttribute("rate", str(context.scene.render.fps))
    for _,keys in data:
        if keys.rotation or keys.position or keys.scale:
            keyset = append_element(anim, "keyset")
            keyset.setAttribute("target", keys.target)
            if keys.rotation: write_keyframes(keyset, "rotation", keys.rotation)
            if keys.position: write_keyframes(keyset, "position", keys.position)
            if keys.scale: write_keyframes(keyset, "scale", keys.scale)

def optimise_keys(keys, identity, compare):
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
        while(True):
            if p in children:
                children[p].append(obj)
                break
            p = p.parent

    print(children);

    # Write xml
    scene = append_element(xml.firstChild, "layout")
    stack = [(None,0, scene)]
    while len(stack)>0:
        print(stack)
        a = stack[-1]
        items = children[a[0]]
        obj = items[a[1]]
        node = write_object(a[2], obj, config)
        if a[1] + 1 >= len(items): stack.pop()
        else: stack[-1] = (a[0], a[1]+1, a[2])

        if children[obj]:
            stack.append( (obj, 0, node) )



def write_object(node, obj, config):
    print("object", obj.name)

    if obj.rotation_mode == 'QUATERNION': r = obj.rotation_quaternion
    elif obj.rotation_mode == 'AXIS_ANGLE': r = Quaternion(obj.rotation_axis_angle[1:4], obj.rotation_axis_angle[0])
    else: r = obj.rotation_euler.to_quaternion()

    rot = (r.w, r.x, r.z, -r.y)
    pos = (obj.location.x, obj.location.z, -obj.location.y)
    scl = obj.scale.xzy.to_tuple()

    node = append_element(node, "object")
    node.setAttribute("name", obj.name)
    if pos != (0,0,0):   node.setAttribute("position", ' '.join( format_num(v) for v in pos ))
    if rot != (1,0,0,0): node.setAttribute("orientation", ' '.join( format_num(v) for v in rot ))
    if scl != (1,1,1):   node.setAttribute("scale", ' '.join( format_num(v) for v in scl ))
    if obj.type == 'MESH': node.setAttribute( "mesh", obj.name if modified(obj,config) else obj.data.name)
    if obj.parent_bone: node.setAttribute("bone", obj.parent_bone)
    
    export_custom_properties(node, obj)

    # Custom properties
    #if len(obj.keys()) > 1:
    #    for key in obj.keys():
    #        if key not in '_RNA_UI':
    #            node.setAttribute(key, str(obj[key]))


    return node


def export_custom_properties(node, obj, name=None):
    if len(obj.keys()) > 1:
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
    if config.export_group == "SELECTED": exportList = list(context.selected_objects)
    elif config.export_group == "HEIRACHY":
        exportList = set()
        for obj in context.selected_objects:
            add_heirachy(obj, exportList)
    elif config.export_group == "ALL": exportList = list(context.view_layer.objects)

    # Linked skeleton
    exportAnimation = config.export_animations != "NONE"
    if config.export_skeletons or exportAnimation:
        for obj in exportList:
            if obj.type == 'MESH':
                arm = obj.find_armature()
                if arm and arm not in exportList:
                    exportList.append(arm)

    xml = create_document("model")
    xml.firstChild.setAttribute("version", "2.0");

    # Need to not duplicate shared meshes
    exportedMeshes = []

    # Export some stuff
    for obj in exportList:
        if obj.type == 'MESH' and config.export_meshes:
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

