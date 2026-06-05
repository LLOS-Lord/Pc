package faddyy.kotakhack.mixin;

import net.minecraft.entity.Entity;
import net.minecraft.util.math.Box;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.Shadow;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfoReturnable;

@Mixin(Entity.class)
public abstract class MixinEntity {
    @Shadow public abstract Box getBoundingBox();

    private static final ThreadLocal<Boolean> expandHitbox = ThreadLocal.withInitial(() -> false);

    public static void enableHitboxExpand() {
        expandHitbox.set(true);
    }

    public static void disableHitboxExpand() {
        expandHitbox.set(false);
    }

    @Inject(method = "getBoundingBox", at = @At("RETURN"), cancellable = true)
    public void onGetBoundingBox(CallbackInfoReturnable<Box> cir) {
        if (expandHitbox.get()) {
            Box original = cir.getReturnValue();
            if (original != null) {
                cir.setReturnValue(original.expand(0.5, 0.5, 0.5));
            }
        }
    }
}