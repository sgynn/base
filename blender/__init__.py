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

    selected_only: BoolProperty(
        name="Export Selected Objects Only",
        description="Export only selected objects",
        default=True)

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
        default=False)

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

    export_animations: BoolProperty(
        name="Export active Animations",
        description="Export active actions of the selection",
        default=True)

    export_all_animations: BoolProperty(
        name="Export all animations",
        description="Export all actions in the scene",
        default=False)

    export_layout: BoolProperty(
        name="Export Layout",
        description="Export object layout",
        default=False)


    def draw(self, context):
        layout = self.layout
        sel = layout.box()
        sel.prop(self, 'selected_only')

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
        skel.enabled = self.hasSkeleton(context)
        skel.prop(self, 'export_skeletons')

        skel = skel.column()
        skel.enabled = self.hasAnimation(context)
        skel.prop(self, 'export_animations')
        skel.prop(self, 'export_all_animations')

        lay = layout.box()
        lay.prop(self, 'export_layout')

    def hasMesh(self, context):
        things = context.selected_objects if self.selected_only else context.view_layer.objects
        for obj in things:
            if obj.type == 'MESH': return True
        return False

    def hasSkeleton(self, context):
        things = context.selected_objects if self.selected_only else context.view_layer.objects
        for obj in things:
            if obj.type == 'ARMATURE': return True
            elif obj.type == 'MESH' and obj.find_armature(): return True
        return False

    def hasAnimation(self, context):
        things = context.selected_objects if self.selected_only else context.view_layer.objects
        for obj in things:
            if obj.type == 'ARMATURE' and obj.animation_data: return True
            elif obj.type == 'MESH' and obj.find_armature():
                if obj.find_armature().animation_data: return True;
        return False


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
