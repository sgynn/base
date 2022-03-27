# ##### BEGIN GPL LICENSE BLOCK #####
#
#  This program is free software; you can redistribute it and/or
#  modify it under the terms of the GNU General Public License
#  as published by the Free Software Foundation, either version 3
#  of the License, or (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program.  If not, see <http://www.gnu.org/licenses/>.
#  All rights reserved.
#
# ##### END GPL LICENSE BLOCK #####

# <pep8 compliant>

bl_info = {
    "name": "BaseLib Format",
    "author": "Sam Gynn",
    "version": (4, 0, 0),
    "blender": (2, 80, 0),
    "location": "File > Export > Base (.bm)",
    "description": "Export to baselib .bm model format",
    "tracker_url": "",
    "category": "Import-Export"}


# Support reloading package
if "bpy" in locals():
    import imp
    if "bmexport" in locals():
        imp.reload(bmexport)


import bpy
from bpy.props import BoolProperty
from bpy.props import EnumProperty
from bpy.props import StringProperty


class ExportBaseLib(bpy.types.Operator):
    """Export selection to BaseLib"""

    bl_idname = "export.bm"
    bl_label = "Export BaseLib"
    bl_options = {'PRESET'}
    filename_ext = ".bm"

    filepath: StringProperty(subtype='FILE_PATH')

    # Export options

    export_group: EnumProperty(
        name="Export objects",
        description="Which objects get exported",
        items = [("ALL", "All", "Export all objects"),
                 ("SELECTED", "Selected", "Only export selected object"),
                 ("HEIRACHY", "Heirachy", "Export selected objects and their children"),
                 ("VISIBLE", "Visible", "Export all visible objects"),
                 ("COLLECTION", "Collection", "Export all objects in collections of which objects are selected")],
        default="SELECTED")

    export_meshes: BoolProperty(
        name="Export Meshes",
        description="Export mesh objects",
        default=True)

    export_normals: BoolProperty(
        name="Export Normals",
        description="Export mesh normals",
        default=True)

    export_tangents: BoolProperty(
        name="Export Tangents",
        description="Export mesh tangents",
        default=True)

    export_uv: BoolProperty(
        name="Export UV Coordinates",
        description="Export mesh UV coordinates, if any",
        default=True)

    export_materials: BoolProperty(
        name="Export Materials",
        description="Export material properties and reference image textures",
        default=True)

    export_colour: BoolProperty(
        name="Export Vertex Colors",
        description="Export mesh vertex colors, if any.\nAlpha channel comes a colour layer names 'alpha'",
        default=False)

    export_skins: BoolProperty(
        name="Export Skin Weights",
        description="Bind mesh vertices to armature bones",
        default=True)

    normalise_weights: BoolProperty(
        name="Normalise Weights",
        description="Normalise weightmaps and limit to a maximum of 4 per vertex",
        default=True)

    apply_modifiers: BoolProperty(
        name="Apply Modifiers",
        description="Apply the effects of object modifiers before export\nThis doesn't apply Armature modifier if skin is exported.",
        default=False)

    apply_transform: BoolProperty(
        name="Apply Transform",
        description="Apply object transforms to mesh data",
        default=False)

    export_skeletons: BoolProperty(
        name="Export Skeletons",
        description="Export armature bones",
        default=True)

    export_animations: EnumProperty(
        name="Animations",
        description="Which animations to export",
        items = [('NONE', "None", "Don't export animation"),
                 ("ACTIVE", "Active", "Export only active action"),
                 ("LINKED", "Linked", "Export actions linked to object (in NLA editor)"),
                 ("UNLOCKED", "Linked Unlocked", "Export actions linked to object in unlocked tracks (in NLA editor)"),
                 ("ALL", "All", "Export all animations")],
        default="NONE")

    animation_names: EnumProperty(
        name="Names",
        description="How to name exported animations",
        items = [("ACTION", "Action Name", "Use action data name"),
                 ("STRIP", "Strip Name", "Use name on NLA strip"),
                 ("TRACK", "Track Name", "Use name of NLA track. Can have multiple animations with the same name")],
        default="ACTION")

    export_layout: BoolProperty(
        name="Export Layout",
        description="Export object layout",
        default=False)


    def draw(self, context):
        layout = self.layout
        sel = layout.box()
        sel.prop(self, 'export_group')

        mesh = layout.box()
        mesh.enabled = self.hasMesh(context)
        mesh.prop(self, 'export_meshes')
        mesh = mesh.column()
        mesh.enabled = self.export_meshes
        mesh.prop(self, 'export_normals')
        mesh.prop(self, 'export_tangents')
        mesh.prop(self, 'export_uv')
        mesh.prop(self, 'export_colour')
        mesh.prop(self, 'export_skins')
        mesh.separator()
        mesh.prop(self, 'normalise_weights')
        mesh.prop(self, 'apply_modifiers')
        mesh.prop(self, 'apply_transform')

        mat = layout.box()
        mat.enabled = self.export_meshes and self.hasMesh(context)
        mat.prop(self, 'export_materials')

        skel = layout.box()
        skel.enabled = self.getSkeleton(context) is not None
        skel.prop(self, 'export_skeletons')

        skel = skel.column()
        skel.enabled = self.hasAnimation(context)
        skel.prop(self, 'export_animations')
        names = skel.column()
        names.enabled = self.export_animations in ("LINKED", "UNLOCKED")
        names.prop(self, 'animation_names')

        lay = layout.box()
        lay.prop(self, 'export_layout')

    def getHeirachy(self, obj, out):
        out.add(obj)
        for o in obj.children: self.getHeirachy(o, out)

    def getObjects(self, context):
        if self.export_group == "ALL": return context.view_layer.objects
        elif self.export_group == "SELECTED": return context.selected_objects
        elif self.export_group == "VISIBLE": return filter(lambda o : not o.hide_viewport, context.view_layer.objects)
        elif self.export_group == "HEIRACHY": 
            items = set();
            for o in context.selected_objects: self.getHeirachy(o, items)
            return items
        elif self.export_group == "COLLECTION":
            items = set()
            collections = set()
            if context.active_object: collections = set(context.active_object.users_collection)
            for obj in context.selected_objects: collections |= set(obj.users_collection)
            for collection in collections: items |= set(collection.objects);
            return items
        else: return None

    def hasMesh(self, context):
        things = self.getObjects(context);
        for obj in things:
            if obj.type == 'MESH': return True
        return False

    def getSkeleton(self, context):
        things = self.getObjects(context);
        for obj in things:
            if obj.type == 'ARMATURE': return obj
            elif obj.type == 'MESH':
                if obj.find_armature(): return obj.find_armature()
                if obj.parent is not None and obj.parent.type == 'ARMATURE': return obj.parent
                for c in obj.constraints:
                    if c.type == 'CHILD_OF' and c.is_valid and c.subtarget: return c.target

        return None

    def hasAnimation(self, context):
        skeleton = self.getSkeleton(context)
        return True if skeleton and skeleton.animation_data else False

    def execute(self, context):
        from . import bmexport
        self.filepath = bpy.path.ensure_ext(self.filepath, ".bm")
        bmexport.export(context, self)
        return {'FINISHED'}

    def invoke(self, context, event):
        if not self.filepath:
            self.filepath = bpy.path.ensure_ext(bpy.data.filepath, ".bm")
        context.window_manager.fileselect_add(self)

        # Disable controls based on selection


        return {'RUNNING_MODAL'}


def menu_func(self, context):
    self.layout.operator(ExportBaseLib.bl_idname, text="Base (.bm)")


def register():
    bpy.utils.register_class(ExportBaseLib)
    bpy.types.TOPBAR_MT_file_export.append(menu_func)


def unregister():
    bpy.types.TOPBAR_MT_file_export.remove(menu_func)
    bpy.utils.unregister_class(ExportBaseLib)


if __name__ == "__main__":
    register()
