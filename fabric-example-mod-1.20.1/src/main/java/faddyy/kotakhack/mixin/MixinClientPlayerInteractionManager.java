package faddyy.kotakhack.mixin;

import faddyy.kotakhack.utils.BlinkManager;
import net.minecraft.client.MinecraftClient;
import net.minecraft.client.network.ClientPlayerInteractionManager;
import net.minecraft.entity.Entity;
import net.minecraft.entity.player.PlayerEntity;
import net.minecraft.network.packet.c2s.play.PlayerMoveC2SPacket;
import net.minecraft.network.packet.c2s.play.PlayerInteractEntityC2SPacket;
import net.minecraft.util.math.Box;
import net.minecraft.util.math.Vec3d;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfoReturnable;

@Mixin(ClientPlayerInteractionManager.class)
public class MixinClientPlayerInteractionManager {
    private static final double MAX_SILENT_REACH = 4.5;
    private static final double MAX_REACH = 6.0;

    @Inject(method = "attackEntity", at = @At("HEAD"), cancellable = true)
    private void onAttackEntity(PlayerEntity player, Entity target, CallbackInfo ci) {
        if (BlinkManager.isBlinking()) return;
        MinecraftClient mc = MinecraftClient.getInstance();
        if (mc.player == null || target == null) return;

        double distance = mc.player.squaredDistanceTo(target);
        if (distance <= MAX_SILENT_REACH * MAX_SILENT_REACH) {
            performSilentAimAttack(mc, target);
        } else {
            BlinkManager.blinkAttack(target);
        }
        ci.cancel();
    }

    private void performSilentAimAttack(MinecraftClient mc, Entity target) {
        float oldYaw = mc.player.getYaw();
        float oldPitch = mc.player.getPitch();

        Vec3d targetPoint = getClosestPointOnBoundingBox(target, mc.player.getEyePos());
        Vec3d dir = targetPoint.subtract(mc.player.getEyePos());
        double yaw = Math.toDegrees(Math.atan2(dir.z, dir.x)) - 90.0;
        double pitch = Math.toDegrees(-Math.atan2(dir.y, Math.sqrt(dir.x * dir.x + dir.z * dir.z)));

        mc.player.setYaw((float) yaw);
        mc.player.setPitch((float) pitch);
        mc.player.networkHandler.sendPacket(new PlayerMoveC2SPacket.LookAndOnGround(mc.player.getYaw(), mc.player.getPitch(), mc.player.isOnGround()));
        mc.player.networkHandler.sendPacket(PlayerInteractEntityC2SPacket.attack(target, mc.player.isSneaking()));

        mc.player.setYaw(oldYaw);
        mc.player.setPitch(oldPitch);
        mc.player.networkHandler.sendPacket(new PlayerMoveC2SPacket.LookAndOnGround(oldYaw, oldPitch, mc.player.isOnGround()));
    }

    private Vec3d getClosestPointOnBoundingBox(Entity entity, Vec3d from) {
        Box bb = entity.getBoundingBox();
        double x = Math.max(bb.minX, Math.min(from.x, bb.maxX));
        double y = Math.max(bb.minY, Math.min(from.y, bb.maxY));
        double z = Math.max(bb.minZ, Math.min(from.z, bb.maxZ));
        return new Vec3d(x, y, z);
    }

    @Inject(method = "hasExtendedReach", at = @At("HEAD"), cancellable = true)
    private void onExtendedReach(CallbackInfoReturnable<Boolean> cir) {
        cir.setReturnValue(true);
    }

    @Inject(method = "getReachDistance", at = @At("RETURN"), cancellable = true)
    private void onGetReachDistance(CallbackInfoReturnable<Float> cir) {
        cir.setReturnValue((float) MAX_REACH);
    }
}