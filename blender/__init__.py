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
    "version": (3, 1, 0),
    "blender": (2, 69, 0),
    "location": "File > Export > Base (.bum)",
    "description": "Export mesh vertices, UV's, materials, textures, "
        "vertex colors, armatures, empties, and actions.",
    "wiki_url": "",
    "tracker_url": "",
    "category": "Import-Export"}


import bpy
from bpy.props import BoolProperty
from bpy.props import EnumProperty
from bpy.props import StringProperty


class ExportBaseLib(bpy.types.Operator):
    """Export selection to BaseLib"""

    bl_idname = "export.bm"
    bl_label = "Export BaseLib"
    bl_options = {'PRESET'}

    filepath = StringProperty(subtype='FILE_PATH')

    # Export options

    SelectedOnly = BoolProperty(
        name="Export Selected Objects Only",
        description="Export only selected objects",
        default=True)

    CoordinateSystem = EnumProperty(
        name="Coordinate System",
        description="Use the selected coordinate system for export",
        items=(('LEFT_HANDED', "Left-Handed", "Use a Y up, Z forward system or a Z up, -Y forward system"),
               ('RIGHT_HANDED', "Right-Handed", "Use a Y up, -Z forward system or a Z up, Y forward system")),
        default='LEFT_HANDED')

    UpAxis = EnumProperty(
        name="Up Axis",
        description="The selected axis points upward",
        items=(('Y', "Y", "The Y axis points up"),
               ('Z', "Z", "The Z axis points up")),
        default='Y')

    ExportMeshes = BoolProperty(
        name="Export Meshes",
        description="Export mesh objects",
        default=True)

    ExportNormals = BoolProperty(
        name="Export Normals",
        description="Export mesh normals",
        default=True)

    FlipNormals = BoolProperty(
        name=" Flip Normals",
        description="Flip mesh normals before export",
        default=False)

    ExportTangents = BoolProperty(
        name="Export Tangents",
        description="Export mesh tangents",
        default=False)

    ExportUVCoordinates = BoolProperty(
        name="Export UV Coordinates",
        description="Export mesh UV coordinates, if any",
        default=True)

    ExportMaterials = BoolProperty(
        name="Export Materials",
        description="Export material properties and reference image textures",
        default=True)

    ExportVertexColors = BoolProperty(
        name="Export Vertex Colors",
        description="Export mesh vertex colors, if any",
        default=False)

    ExportSkinWeights = BoolProperty(
        name="Export Skin Weights",
        description="Bind mesh vertices to armature bones",
        default=True)

    ApplyModifiers = BoolProperty(
        name="Apply Modifiers",
        description="Apply the effects of object modifiers before export\nThis doesn't apply Armature modifier if skin is exported.",
        default=False)

    ApplyTransform = BoolProperty(
        name="Apply Transform",
        description="Apply object transforms to mesh data",
        default=False)

    ExportArmatureBones = BoolProperty(
        name="Export Skeletons",
        description="Export armature bones",
        default=True)

    ExportAnimation = BoolProperty(
        name="Export Animations",
        description="Export object and bone animations.",
        default=True)

    AttachToFirstArmature = BoolProperty(
        name="Export unused animations",
        description="Export each unused action as if used by the first armature object",
        default=False)

    Verbose = BoolProperty(
        name="Verbose",
        description="Run the exporter in debug mode. Check the console for "\
            "output",
        default=True)

    def draw(self, context):
        layout = self.layout
        sel = layout.box()
        sel.prop(self, 'SelectedOnly')

        mesh = layout.box()
        mesh.prop(self, 'ExportMeshes')
        mesh.prop(self, 'ExportNormals')
        mesh.prop(self, 'ExportTangents')
        mesh.prop(self, 'ExportUVCoordinates')
        mesh.prop(self, 'ExportVertexColors')
        mesh.prop(self, 'ExportSkinWeights')
        mesh.separator()
        mesh.prop(self, 'ApplyModifiers')
        mesh.prop(self, 'ApplyTransform')

        mat = layout.box()
        mat.prop(self, 'ExportMaterials')

        skel = layout.box()
        skel.prop(self, 'ExportArmatureBones')
        skel.prop(self, 'ExportAnimation')
        skel.prop(self, 'AttachToFirstArmature')

        main = layout.box()
        main.prop(self, 'CoordinateSystem')
        main.prop(self, 'UpAxis')
        main.prop(self, 'Verbose')


    def execute(self, context):
        self.filepath = bpy.path.ensure_ext(self.filepath, ".bm")

        from . import export_bm
        Exporter = export_bm.BMExporter(self, context)
        Exporter.Export()
        return {'FINISHED'}

    def invoke(self, context, event):
        if not self.filepath:
            self.filepath = bpy.path.ensure_ext(bpy.data.filepath, ".bm")
        context.window_manager.fileselect_add(self)
        return {'RUNNING_MODAL'}


def menu_func(self, context):
    self.layout.operator(ExportBaseLib.bl_idname, text="Base (.bm)")


def register():
    bpy.utils.register_module(__name__)

    bpy.types.INFO_MT_file_export.append(menu_func)


def unregister():
    bpy.utils.unregister_module(__name__)

    bpy.types.INFO_MT_file_export.remove(menu_func)


if __name__ == "__main__":
    register()
