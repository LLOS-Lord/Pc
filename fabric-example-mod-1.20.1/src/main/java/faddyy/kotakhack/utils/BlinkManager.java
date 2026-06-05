package faddyy.kotakhack.utils;

import net.minecraft.client.MinecraftClient;
import net.minecraft.entity.Entity;
import net.minecraft.network.packet.Packet;
import net.minecraft.network.packet.c2s.play.PlayerMoveC2SPacket;
import net.minecraft.network.packet.c2s.play.PlayerInteractEntityC2SPacket;
import net.minecraft.util.math.Vec3d;
import java.util.LinkedList;
import java.util.Queue;

public class BlinkManager {
    private static final Queue<Packet<?>> delayedPackets = new LinkedList<>();
    private static boolean blinking = false;

    public static void startBlink() {
        blinking = true;
        delayedPackets.clear();
    }

    public static void stopBlink() {
        blinking = false;
    }

    public static boolean isBlinking() {
        return blinking;
    }

    // Được gọi từ MixinClientConnection mỗi khi client gửi packet
    public static boolean onPacketSend(Packet<?> packet) {
        if (!blinking) return false;
        if (packet instanceof PlayerMoveC2SPacket) {
            delayedPackets.add(packet);
            return true; // Chặn packet di chuyển
        }
        return false; // Cho phép các packet khác (attack, look,...) đi qua
    }

    // Gửi tất cả packet bị trì hoãn sau khi kết thúc blink
    public static void flushDelayedPackets() {
        MinecraftClient mc = MinecraftClient.getInstance();
        if (mc.getNetworkHandler() == null) return;
        while (!delayedPackets.isEmpty()) {
            mc.getNetworkHandler().sendPacket(delayedPackets.poll());
        }
    }

    // Thực hiện tấn công blink: teleport gần victim, đánh, rồi trở về vị trí cũ
    public static void blinkAttack(Entity victim) {
        MinecraftClient mc = MinecraftClient.getInstance();
        if (mc.player == null || victim == null) return;

        Vec3d targetPos = victim.getPos();
        Vec3d eyeHeightVector = new Vec3d(0, mc.player.getStandingEyeHeight(), 0);
        Vec3d playerPos = mc.player.getPos();
        Vec3d direction = playerPos.subtract(targetPos).normalize();
        // Đặt vị trí tấn công cách victim 2 block về phía người chơi
        Vec3d attackPos = targetPos.add(direction.multiply(2.0));
        attackPos = new Vec3d(attackPos.x, targetPos.y, attackPos.z); // đứng cùng cao độ
        Vec3d oldPos = playerPos;
        boolean oldOnGround = mc.player.isOnGround();

        startBlink();

        // Gửi packet position đến vị trí gần
        mc.getNetworkHandler().sendPacket(new PlayerMoveC2SPacket.PositionAndOnGround(attackPos.x, attackPos.y, attackPos.z, false));

        // Tính rotation chính xác từ vị trí giả đến victim
        Vec3d eyeAtAttack = attackPos.add(0, mc.player.getStandingEyeHeight(), 0);
        Vec3d targetEye = targetPos.add(0, victim.getHeight() / 2.0, 0);
        Vec3d dir = targetEye.subtract(eyeAtAttack);
        float yaw = (float) Math.toDegrees(Math.atan2(dir.z, dir.x)) - 90.0f;
        float pitch = (float) Math.toDegrees(-Math.atan2(dir.y, Math.sqrt(dir.x * dir.x + dir.z * dir.z)));

        // Gửi look packet (phải khớp với hướng tấn công)
        mc.getNetworkHandler().sendPacket(new PlayerMoveC2SPacket.LookAndOnGround(yaw, pitch, false));

        // Gửi packet tấn công
        mc.getNetworkHandler().sendPacket(PlayerInteractEntityC2SPacket.attack(victim, mc.player.isSneaking()));

        // Gửi packet quay trở về vị trí cũ và rotation cũ
        mc.getNetworkHandler().sendPacket(new PlayerMoveC2SPacket.PositionAndOnGround(oldPos.x, oldPos.y, oldPos.z, oldOnGround));
        float oldYaw = mc.player.getYaw();
        float oldPitch = mc.player.getPitch();
        mc.getNetworkHandler().sendPacket(new PlayerMoveC2SPacket.LookAndOnGround(oldYaw, oldPitch, oldOnGround));

        stopBlink();
        flushDelayedPackets(); // Gửi nốt các packet move bị chặn trong lúc blink
    }
}