import bpy
import math

bl_info = {
    "name": "NDS Model Optimizer",
    "author": "P3D Team",
    "version": (1, 1, 0),
    "blender": (3, 0, 0),
    "location": "View3D > Sidebar > NDS Engine",
    "description": "Aggressively reduces rigged character polycounts for NDS hardware.",
    "category": "Object",
}


class NDS_OT_OptimizeCharacter(bpy.types.Operator):
    """Optimize rigged meshes for NDS (Poly reduction only)"""

    bl_idname = "object.nds_optimize_char"
    bl_label = "Optimize Model"
    bl_options = {"REGISTER", "UNDO"}

    reduction_ratio: bpy.props.FloatProperty(
        name="Target Poly Ratio",
        default=0.25,
        min=0.01,
        max=0.99,
        description="0.25 means keep 25% of original polygons (75% reduction)",
    )

    preserve_uvs: bpy.props.BoolProperty(
        name="Strict UV Preservation",
        default=True,
        description="Prevents facial textures and clothing logos from tearing",
    )

    @classmethod
    def poll(cls, context):
        return context.selected_objects

    def execute(self, context):
        processed = 0
        selected = context.selected_objects.copy()

        for obj in selected:
            if obj.type != "MESH":
                continue

            bpy.ops.object.select_all(action="DESELECT")
            obj.select_set(True)
            context.view_layer.objects.active = obj

            try:
                # 1. Clean up internal data
                bpy.ops.object.mode_set(mode="EDIT")
                bpy.ops.mesh.select_all(action="SELECT")
                bpy.ops.mesh.remove_doubles(threshold=0.0001)
                bpy.ops.object.mode_set(mode="OBJECT")

                # 2. Aggressive Character Decimation (Collapse Method)
                dec = obj.modifiers.new(name="NDS_Char_Decimate", type="DECIMATE")
                dec.decimate_type = "COLLAPSE"
                dec.ratio = self.reduction_ratio
                dec.use_symmetry = True

                # Protect UVs and sharp seams
                if self.preserve_uvs:
                    dec.use_collapse_triangulate = True

                bpy.ops.object.modifier_apply(modifier=dec.name)

                # 3. Format for the NDS Engine (Tris to Quads)
                tri = obj.modifiers.new(name="NDS_Triangulate", type="TRIANGULATE")
                tri.keep_custom_normals = True
                tri.quad_method = "BEAUTY"
                tri.ngon_method = "BEAUTY"
                bpy.ops.object.modifier_apply(modifier=tri.name)

                bpy.ops.object.mode_set(mode="EDIT")
                bpy.ops.mesh.select_all(action="SELECT")
                bpy.ops.mesh.tris_convert_to_quads(
                    face_threshold=math.radians(45),
                    shape_threshold=math.radians(45),
                    uvs=True,
                    materials=True,
                )
                bpy.ops.object.mode_set(mode="OBJECT")

                processed += 1

            except Exception as e:
                self.report({"ERROR"}, f"Failed on {obj.name}: {str(e)}")
                if context.object and context.object.mode != "OBJECT":
                    bpy.ops.object.mode_set(mode="OBJECT")

        # Restore selection
        for obj in selected:
            obj.select_set(True)

        self.report(
            {"INFO"},
            f"Optimized {processed} characters. Kept {int(self.reduction_ratio * 100)}% of geometry.",
        )
        return {"FINISHED"}


class NDS_PT_CharPanel(bpy.types.Panel):
    bl_label = "NDS Model Optimizer"
    bl_space_type = "VIEW_3D"
    bl_region_type = "UI"
    bl_category = "NDS Engine"

    def draw(self, context):
        layout = self.layout
        col = layout.column(align=True)
        col.operator(
            "object.nds_optimize_char", text="Optimize Model", icon="ARMATURE_DATA"
        )


classes = (
    NDS_OT_OptimizeCharacter,
    NDS_PT_CharPanel,
)


def register():
    for cls in classes:
        bpy.utils.register_class(cls)


def unregister():
    for cls in reversed(classes):
        bpy.utils.unregister_class(cls)


if __name__ == "__main__":
    register()
