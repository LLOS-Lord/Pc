package faddyy.kotakhack.mixin;

import net.minecraft.entity.Entity;
import net.minecraft.util.hit.HitResult;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfoReturnable;

@Mixin(Entity.class)
public class MixinEntityRaycast {
    @Inject(method = "raycast", at = @At("HEAD"))
    private static void onRaycastStart(double maxDistance, float tickDelta, boolean includeFluids, CallbackInfoReturnable<HitResult> cir) {
        MixinEntity.enableHitboxExpand();
    }

    @Inject(method = "raycast", at = @At("RETURN"))
    private static void onRaycastEnd(double maxDistance, float tickDelta, boolean includeFluids, CallbackInfoReturnable<HitResult> cir) {
        MixinEntity.disableHitboxExpand();
    }
}