import bpy
import math

bl_info = {
    "name": "NDS Environment Optimizer",
    "author": "P3D Team",
    "version": (1, 0, 0),
    "blender": (3, 0, 0),
    "location": "View3D > Sidebar > NDS Engine",
    "description": "Safely optimizes 3D environments for NDS hardware while bypassing flat 2D planes.",
    "category": "Object",
}


class NDS_OT_Optimize(bpy.types.Operator):
    """Optimize NDS meshes (Adjustable bypass filters for 2D/small props)"""

    bl_idname = "object.nds_optimize"
    bl_label = "Optimize Environment"
    bl_options = {"REGISTER", "UNDO"}

    angle_limit: bpy.props.FloatProperty(
        name="Planar Angle",
        default=5.0,
        min=0.0,
        max=45.0,
        description="Higher values reduce more polygons but destroy curves",
    )

    ignore_poly_count: bpy.props.IntProperty(
        name="Ignore Polys Below",
        default=4,
        min=0,
        max=100,
        description="Skip objects with fewer polygons than this (keeps billboards safe)",
    )

    flatness_threshold: bpy.props.FloatProperty(
        name="Flatness (Thickness)",
        default=0.005,
        min=0.0001,
        max=0.5,
        description="If the object is thinner than this, it is treated as a 2D plane and skipped",
    )

    @classmethod
    def poll(cls, context):
        return context.selected_objects

    def execute(self, context):
        processed = 0
        bypassed = 0
        original_selection = context.selected_objects.copy()

        for base_obj in original_selection:
            if base_obj.type != "MESH":
                continue

            original_name = base_obj.name

            bpy.ops.object.select_all(action="DESELECT")
            base_obj.select_set(True)
            context.view_layer.objects.active = base_obj

            try:
                bpy.ops.object.transform_apply(
                    location=False, rotation=True, scale=True
                )

                bpy.ops.object.mode_set(mode="EDIT")
                bpy.ops.mesh.separate(type="LOOSE")
                bpy.ops.object.mode_set(mode="OBJECT")

                loose_parts = context.selected_objects.copy()

                for part in loose_parts:
                    bpy.ops.object.select_all(action="DESELECT")
                    part.select_set(True)
                    context.view_layer.objects.active = part

                    # Bounding Box Thickness Check
                    bb = part.bound_box
                    dx = max(v[0] for v in bb) - min(v[0] for v in bb)
                    dy = max(v[1] for v in bb) - min(v[1] for v in bb)
                    dz = max(v[2] for v in bb) - min(v[2] for v in bb)

                    is_flat = min(dx, dy, dz) <= self.flatness_threshold
                    is_low_poly = len(part.data.polygons) <= self.ignore_poly_count

                    if not is_flat and not is_low_poly:
                        dec = part.modifiers.new(name="NDS_Planar", type="DECIMATE")
                        dec.decimate_type = "DISSOLVE"
                        dec.angle_limit = math.radians(self.angle_limit)
                        dec.delimit = {"NORMAL", "MATERIAL", "UV"}
                        bpy.ops.object.modifier_apply(modifier=dec.name)
                    else:
                        bypassed += 1

                    tri = part.modifiers.new(name="NDS_Triangulate", type="TRIANGULATE")
                    tri.keep_custom_normals = True
                    tri.quad_method = "BEAUTY"
                    tri.ngon_method = "CLIP"
                    bpy.ops.object.modifier_apply(modifier=tri.name)

                    bpy.ops.object.mode_set(mode="EDIT")
                    bpy.ops.mesh.select_all(action="SELECT")
                    bpy.ops.mesh.tris_convert_to_quads(
                        face_threshold=math.radians(40),
                        shape_threshold=math.radians(40),
                        uvs=True,
                        materials=True,
                    )
                    bpy.ops.object.mode_set(mode="OBJECT")

                bpy.ops.object.select_all(action="DESELECT")
                for part in loose_parts:
                    part.select_set(True)

                context.view_layer.objects.active = loose_parts[0]
                if len(loose_parts) > 1:
                    bpy.ops.object.join()

                context.view_layer.objects.active.name = original_name
                processed += 1

            except Exception as e:
                self.report({"ERROR"}, f"Failed on {original_name}: {str(e)}")
                if context.object and context.object.mode != "OBJECT":
                    bpy.ops.object.mode_set(mode="OBJECT")

        bpy.ops.object.select_all(action="DESELECT")
        for obj in context.scene.objects:
            if obj.name in [o.name for o in original_selection]:
                obj.select_set(True)

        self.report(
            {"INFO"},
            f"Optimized {processed} groups. Bypassed {bypassed} flat/tiny parts.",
        )
        return {"FINISHED"}


class NDS_PT_Panel(bpy.types.Panel):
    bl_label = "NDS Environment Optimizer"
    bl_space_type = "VIEW_3D"
    bl_region_type = "UI"
    bl_category = "NDS Engine"

    def draw(self, context):
        layout = self.layout
        col = layout.column(align=True)
        col.operator(
            "object.nds_optimize", text="Optimize Environment", icon="MOD_DECIM"
        )


classes = (
    NDS_OT_Optimize,
    NDS_PT_Panel,
)


def register():
    for cls in classes:
        bpy.utils.register_class(cls)


def unregister():
    for cls in reversed(classes):
        bpy.utils.unregister_class(cls)


if __name__ == "__main__":
    register()
