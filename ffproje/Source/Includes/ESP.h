#pragma once
#import <UIKit/UIKit.h>
#import <Foundation/Foundation.h>
#import <QuartzCore/QuartzCore.h>
#import <substrate.h>

#import "UnityTypes.h"
#import "Vector3.h"
#import "Vector2.h"
#import "Quaternion.h"
#import "MemoryUtils.h"

// ===== BASIC STRUCTURES =====
struct SimpleVec2 {
    float x, y;
    SimpleVec2() : x(0), y(0) {}
    SimpleVec2(float x, float y) : x(x), y(y) {}
};

// Unified Red Color Constant (iOS 9+ Compatible)
#define kUnifiedRedColor [UIColor redColor].CGColor

// ===== SIMPLIFIED VARIABLES =====
struct Vars_t
{
    bool Enable = true;
    bool lines = false;
    bool Box = false;
    bool Name = false;
    bool Health = false;
    bool Distance = false;
    bool skeleton = false;
    bool NoFog = true;

    // Aimbot
    bool Aimbot = false;
    bool ShowFovCircle = true;   // دائماً مفعّل مع Aimbot
    bool VisibleCheck = true;    // ثابت true
    bool IgnoreKnocked = true;   // ثابت true
    float AimFov = 150.0f;
    int AimHitbox = 0;     // 0=Head 1=Neck 2=Body
    int AimWhen = 3;       // 0=Always 1=Fire 2=Scope 3=Fire+Scope
} Vars;

// ===== GAME SDK =====
class game_sdk_t
{
public:
    void init();
    void *(*Curent_Match)();
    void *(*GetLocalPlayer)(void *Game);
    Vector3 (*get_position)(void *player);
    void *(*Component_GetTransform)(void *player);
    void *(*get_camera)();
    Vector3 (*WorldToScreenPoint)(void *, Vector3);
    Vector3 (*WorldToScreenPointEx)(void *, Vector3, int);
    Vector3 (*GetForward)(void *player);
    bool (*get_isLocalTeam)(void *player);
    bool (*get_IsDieing)(void *player);
    bool (*get_isVisible)(void *player);
    bool (*get_IsSighting)(void *player);
    bool (*get_IsFiring)(void *player);
    int (*get_MaxHP)(void *player);
    int (*GetHp)(void *player);
    void (*set_aim)(void *, Quaternion);
    monoString *(*name)(void *player);
    void *(*GetHeadPositions)(void *player);

    void *(*_GetHeadPositions)(void *);
    void *(*_newHipMods)(void *);
    void *(*_GetLeftAnkleTF)(void *);
    void *(*_GetRightAnkleTF)(void *);
    void *(*_GetLeftToeTF)(void *);
    void *(*_GetRightToeTF)(void *);
    void *(*_getLeftHandTF)(void *);
    void *(*_getRightHandTF)(void *);
    void *(*_getLeftForeArmTF)(void *);
    void *(*_getRightForeArmTF)(void *);
};

extern game_sdk_t *game_sdk;

// ===== WORLD TO SCREEN HELPER =====
namespace Camera$$WorldToScreen
{
    inline SimpleVec2 Regular(Vector3 pos)
    {
        auto cam = game_sdk->get_camera();
        if (!cam) return SimpleVec2(0, 0);
        Vector3 worldPoint = game_sdk->WorldToScreenPoint(cam, pos);
        
        if (worldPoint.z < 0.01f) return SimpleVec2(0, 0);

        CGRect screenBounds = [UIScreen mainScreen].nativeBounds;
        CGFloat screenWidth = screenBounds.size.width / [UIScreen mainScreen].nativeScale;
        CGFloat screenHeight = screenBounds.size.height / [UIScreen mainScreen].nativeScale;
        
        if (screenWidth < screenHeight) {
            CGFloat temp = screenWidth;
            screenWidth = screenHeight;
            screenHeight = temp;
        }

        float lx = screenWidth * worldPoint.x;
        float ly = screenHeight * (1.0f - worldPoint.y);
        
        return SimpleVec2(lx, ly);
    }
}

// ===== HELPERS =====
inline Vector3 getPosition(void *transform)
{
    if (!transform) return Vector3();
    return game_sdk->get_position(game_sdk->Component_GetTransform(transform));
}

inline Vector3 GetBonePosition(void *player, void *(*transformGetter)(void *)) {
    if (!player || !transformGetter) return Vector3();
    void *transform = transformGetter(player);
    if (!transform) return Vector3();
    void *tf = game_sdk->Component_GetTransform(transform);
    return tf ? game_sdk->get_position(tf) : Vector3();
}

// ===== OPTIMIZED ESP RENDERER =====
@interface ESPRenderer : NSObject
@property (nonatomic, strong) CAShapeLayer *espLayer;
@property (nonatomic, strong) UIView *containerView;
@property (nonatomic, strong) NSMutableArray<CATextLayer *> *textPool;
@property (nonatomic, strong) NSMutableArray<CALayer *> *healthPool;
@property (nonatomic, assign) int textUsedCount;
@property (nonatomic, assign) int healthUsedCount;

+ (instancetype)sharedInstance;
- (void)renderOnView:(UIView *)view;
- (void)clearDrawings;
- (void)drawBoxFrom:(SimpleVec2)min to:(SimpleVec2)max path:(UIBezierPath *)path;
- (void)drawLineFrom:(SimpleVec2)from to:(SimpleVec2)to path:(UIBezierPath *)path;
- (void)drawTextAt:(SimpleVec2)position text:(NSString*)text;
- (void)drawHealthBarAt:(SimpleVec2)min to:(SimpleVec2)max multiplier:(float)multiplier;
@end

@implementation ESPRenderer

+ (instancetype)sharedInstance {
    static ESPRenderer *instance = nil;
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        instance = [[self alloc] init];
    });
    return instance;
}

- (instancetype)init {
    self = [super init];
    if (self) {
        _espLayer = [CAShapeLayer layer];
        _espLayer.name = @"ESP_Layer_Optimized";
        _espLayer.fillColor = [UIColor clearColor].CGColor;
        _espLayer.strokeColor = kUnifiedRedColor;
        _espLayer.lineWidth = 1.2f;
        _textPool = [NSMutableArray new];
        _healthPool = [NSMutableArray new];
    }
    return self;
}

- (void)renderOnView:(UIView *)view {
    if (!view) return;
    if (!_containerView || _containerView != view) {
        _containerView = view;
        [_containerView.layer addSublayer:_espLayer];
    }
    _espLayer.frame = view.bounds;
    _textUsedCount = 0;
    _healthUsedCount = 0;
}

- (void)clearDrawings {
    _espLayer.path = nil;
    for (CATextLayer *layer in _textPool) layer.hidden = YES;
    for (CALayer *layer in _healthPool) layer.hidden = YES;
}

- (void)drawBoxFrom:(SimpleVec2)min to:(SimpleVec2)max path:(UIBezierPath *)path {
    if (!path) return;
    float w = max.x - min.x;
    float h = max.y - min.y;
    float cw = w * 0.25f; // طول الزاوية
    float ch = h * 0.25f;
    // Top-left
    [path moveToPoint:CGPointMake(min.x, min.y + ch)];
    [path addLineToPoint:CGPointMake(min.x, min.y)];
    [path addLineToPoint:CGPointMake(min.x + cw, min.y)];
    // Top-right
    [path moveToPoint:CGPointMake(max.x - cw, min.y)];
    [path addLineToPoint:CGPointMake(max.x, min.y)];
    [path addLineToPoint:CGPointMake(max.x, min.y + ch)];
    // Bottom-left
    [path moveToPoint:CGPointMake(min.x, max.y - ch)];
    [path addLineToPoint:CGPointMake(min.x, max.y)];
    [path addLineToPoint:CGPointMake(min.x + cw, max.y)];
    // Bottom-right
    [path moveToPoint:CGPointMake(max.x - cw, max.y)];
    [path addLineToPoint:CGPointMake(max.x, max.y)];
    [path addLineToPoint:CGPointMake(max.x, max.y - ch)];
}

- (void)drawLineFrom:(SimpleVec2)from to:(SimpleVec2)to path:(UIBezierPath *)path {
    if (!path) return;
    [path moveToPoint:CGPointMake(from.x, from.y)];
    [path addLineToPoint:CGPointMake(to.x, to.y)];
}

- (void)drawTextAt:(SimpleVec2)position text:(NSString*)text {
    if (!text || !_containerView) return;
    CATextLayer *textLayer;
    if (_textUsedCount < _textPool.count) {
        textLayer = _textPool[_textUsedCount];
    } else {
        textLayer = [CATextLayer layer];
        textLayer.fontSize = 10.0f;
        textLayer.foregroundColor = [UIColor whiteColor].CGColor;
        textLayer.backgroundColor = [UIColor colorWithWhite:0 alpha:0.45].CGColor;
        textLayer.cornerRadius = 2.0f;
        textLayer.alignmentMode = kCAAlignmentCenter;
        textLayer.contentsScale = [UIScreen mainScreen].scale;
        [_containerView.layer addSublayer:textLayer];
        [_textPool addObject:textLayer];
    }
    textLayer.string = text;
    textLayer.hidden = NO;
    CGSize textSize = [text sizeWithAttributes:@{NSFontAttributeName: [UIFont systemFontOfSize:9.0f]}];
    textLayer.frame = CGRectMake(position.x - textSize.width/2, position.y, textSize.width + 4, textSize.height + 2);
    _textUsedCount++;
}

- (void)drawHealthBarAt:(SimpleVec2)min to:(SimpleVec2)max multiplier:(float)multiplier {
    if (!_containerView) return;
    CALayer *bgLayer;
    CALayer *fgLayer;
    int index = _healthUsedCount * 2;
    
    if (index + 1 < _healthPool.count) {
        bgLayer = _healthPool[index];
        fgLayer = _healthPool[index + 1];
    } else {
        bgLayer = [CALayer layer];
        bgLayer.backgroundColor = [UIColor colorWithWhite:0.0 alpha:0.6].CGColor;
        [_containerView.layer addSublayer:bgLayer];
        [_healthPool addObject:bgLayer];
        
        fgLayer = [CALayer layer];
        [_containerView.layer addSublayer:fgLayer];
        [_healthPool addObject:fgLayer];
    }
    
    // لون ديناميكي حسب الـ HP
    UIColor *hpColor;
    if (multiplier > 0.6f)      hpColor = [UIColor colorWithRed:0.2 green:1.0 blue:0.2 alpha:1.0]; // أخضر
    else if (multiplier > 0.3f) hpColor = [UIColor colorWithRed:1.0 green:0.85 blue:0.0 alpha:1.0]; // أصفر
    else                        hpColor = [UIColor colorWithRed:1.0 green:0.1 blue:0.1 alpha:1.0]; // أحمر
    
    fgLayer.backgroundColor = hpColor.CGColor;
    
    float height = max.y - min.y;
    float barW = 3.0f;  // أعرض من 2
    bgLayer.frame = CGRectMake(min.x, min.y, barW, height);
    fgLayer.frame = CGRectMake(min.x, max.y - (height * multiplier), barW, height * multiplier);
    bgLayer.hidden = NO;
    fgLayer.hidden = NO;
    _healthUsedCount++;
}

@end

// ===== AIMBOT HELPERS =====
static void *Player_GetHeadCollider(void *p)
    { return ((void*(*)(void*))getRealOffset(0x4A1A9D4))(p); }
static Vector3 Transform_INTERNAL_GetPosition(void *tf) {
    Vector3 o = Vector3();
    ((void(*)(void*,Vector3*))getRealOffset(0x8552C10))(tf,&o);
    return o;
}
static bool Physics_Raycast(Vector3 a, Vector3 b, unsigned int l, void *c)
    { return ((bool(*)(Vector3,Vector3,unsigned int,void*))getRealOffset(0x5580870))(a,b,l,c); }
static void *getItransform(void *itf)
    { return ((void*(*)(void*))getRealOffset(0x5C52CFC))(itf); }

inline Vector3 GetHeadPosition(void *p) {
    if (!p || !game_sdk->GetHeadPositions) return Vector3();
    void *h = game_sdk->GetHeadPositions(p);
    return h ? game_sdk->get_position(h) : Vector3();
}
inline Vector3 GetHipPosition(void *p) {
    if (!p) return Vector3();
    void *HipITF = *(void**)((uint64_t)p + 0x648);
    if (!HipITF) return Vector3();
    void *HipTF = getItransform(HipITF);
    return HipTF ? Transform_INTERNAL_GetPosition(HipTF) : Vector3();
}
inline Vector3 CameraMain(void *p) {
    if (!p) return Vector3();
    void *camTF = *(void**)((uint64_t)p + 0x390);
    return camTF ? game_sdk->get_position(camTF) : Vector3();
}
inline bool IsGod(void *p) { return *(bool*)((uint64_t)p + 0xF4C); }

inline Vector3 GetHitboxPosition(void *p, int hitbox) {
    Vector3 h = GetHeadPosition(p);
    if (h.x == 0 && h.y == 0 && h.z == 0) return h;
    if (hitbox == 1) return Vector3(h.x, h.y - 0.2f, h.z);
    if (hitbox == 2) return Vector3(h.x, h.y - 0.4f, h.z);
    return h;
}

inline bool ff_isVisible(void *enemy) {
    if (!enemy) return false;
    void *hitObj = nullptr;
    void *camTF = game_sdk->Component_GetTransform(game_sdk->get_camera());
    void *col   = Player_GetHeadCollider(enemy);
    if (!camTF || !col) return false;
    void *colTF = game_sdk->Component_GetTransform(col);
    if (!colTF) return false;
    Vector3 cam  = Transform_INTERNAL_GetPosition(camTF);
    Vector3 head = Transform_INTERNAL_GetPosition(colTF);
    return !Physics_Raycast(cam, head, 12, &hitObj);
}


// ===== AIMBOT - Get Closest Enemy =====
inline void *GetClosestEnemy() {
    void *match = game_sdk->Curent_Match(); if (!match) return nullptr;
    void *local = game_sdk->GetLocalPlayer(match);
    if (!local || !game_sdk->Component_GetTransform(local)) return nullptr;

    void *playersListAddr = (void*)getRealOffset(0x4C869DC);
    if (!playersListAddr) return nullptr;
    monoList<void **> *players = ((monoList<void **>* (*)(void*))playersListAddr)(match);
    if (!players || !players->getItems()) return nullptr;

    float best = 9999.f; void *target = nullptr;
    Vector3 localPos = getPosition(local);
    float cx = [UIScreen mainScreen].bounds.size.width / 2.f;
    float cy = [UIScreen mainScreen].bounds.size.height / 2.f;

    for (int i = 0; i < players->getSize(); i++) {
        void *p = players->getItems()[i];
        if (!p || p == local) continue;
        if (!game_sdk->get_MaxHP(p)) continue;
        if (Vars.IgnoreKnocked && game_sdk->get_IsDieing(p)) continue;
        if (game_sdk->get_isLocalTeam(p)) continue;
        if (game_sdk->get_isVisible && !game_sdk->get_isVisible(p)) continue;

        Vector3 pos = getPosition(p);
        if (Vector3::Distance(localPos, pos) > 200.f) continue;

        SimpleVec2 sc = Camera$$WorldToScreen::Regular(pos);
        if (sc.x == 0 && sc.y == 0) continue;
        float dx = sc.x - cx, dy = sc.y - cy;
        float fov = sqrtf(dx*dx + dy*dy);
        if (fov > Vars.AimFov) continue;

        if (Vars.VisibleCheck && !ff_isVisible(p)) continue;
        if (fov < best) { best = fov; target = p; }
    }
    return target;
}

// ===== AIMBOT - Process =====
inline void ProcessAimbot() {
    if (!Vars.Aimbot || !game_sdk->set_aim) return;
    void *match = game_sdk->Curent_Match(); if (!match) return;
    void *local = game_sdk->GetLocalPlayer(match);
    if (!local || !game_sdk->Component_GetTransform(local)) return;

    bool fire  = game_sdk->get_IsFiring  ? game_sdk->get_IsFiring(local)  : false;
    bool scope = game_sdk->get_IsSighting? game_sdk->get_IsSighting(local): false;
    bool should = (Vars.AimWhen == 0) ||
                  (Vars.AimWhen == 1 && fire) ||
                  (Vars.AimWhen == 2 && scope) ||
                  (Vars.AimWhen == 3 && (fire || scope));
    if (!should) return;

    void *enemy = GetClosestEnemy(); if (!enemy) return;
    Vector3 enemyPos = GetHitboxPosition(enemy, Vars.AimHitbox);
    Vector3 camPos = CameraMain(local);
    if ((enemyPos.x==0&&enemyPos.y==0&&enemyPos.z==0)||
        (camPos.x==0&&camPos.y==0&&camPos.z==0)) return;

    Vector3 dir = (enemyPos + Vector3(0,0.05f,0)) - camPos;
    Quaternion q = Quaternion::LookRotation(dir, Vector3(0,1,0));
    game_sdk->set_aim(local, q);
}

// ===== FOV CIRCLE LAYER =====
@interface FOVCircleManager : NSObject
@property (nonatomic, strong) CAShapeLayer *fovLayer;
+ (instancetype)shared;
- (void)updateOnView:(UIView*)view;
@end

@implementation FOVCircleManager
+ (instancetype)shared {
    static FOVCircleManager *i; static dispatch_once_t t;
    dispatch_once(&t,^{ i=[self new]; });
    return i;
}
- (instancetype)init {
    self=[super init];
    _fovLayer=[CAShapeLayer layer];
    _fovLayer.fillColor=UIColor.clearColor.CGColor;
    _fovLayer.strokeColor=[[UIColor whiteColor] colorWithAlphaComponent:0.85].CGColor;
    _fovLayer.lineWidth=1.0f;
    return self;
}
- (void)updateOnView:(UIView*)view {
    if (!view) return;
    if (_fovLayer.superlayer != view.layer) [view.layer addSublayer:_fovLayer];
    _fovLayer.frame = view.bounds;
    if (Vars.Aimbot && Vars.ShowFovCircle) {
        float r = Vars.AimFov;
        float cx = view.bounds.size.width/2.f;
        float cy = view.bounds.size.height/2.f;
        _fovLayer.path = [UIBezierPath bezierPathWithOvalInRect:
            CGRectMake(cx-r,cy-r,r*2,r*2)].CGPath;
    } else _fovLayer.path = nil;
}
@end

// ===== MAIN ESP FUNCTION =====
inline void get_players()
{
    // Safety check for game_sdk
    if (!game_sdk) return;

    // Ensure drawing happens on Main Thread
    dispatch_async(dispatch_get_main_queue(), ^{
        if (!Vars.Enable) {
            [[ESPRenderer sharedInstance] clearDrawings];
            return;
        }

        // No Fog (auto)
        if (Vars.NoFog) {
            void *noFogAddr = (void*)getRealOffset(0x850363C);
            if (noFogAddr) ((void (*)(bool))noFogAddr)(false);
        }

        void *current_Match = game_sdk->Curent_Match();
        if (!current_Match) {
            [[ESPRenderer sharedInstance] clearDrawings];
            return;
        }

        // Process aimbot
        ProcessAimbot();

        void *local_player = game_sdk->GetLocalPlayer(current_Match);
        if (!local_player) return;

        // الطريقة الأصلية للمشروع - كانت شغّالة
        void *playersListAddr = (void*)getRealOffset(0x4C869DC);
        if (!playersListAddr) return;
        
        monoList<void **> *players = ((monoList<void **>* (*)(void*))playersListAddr)(current_Match);
        if (!players || !players->getItems()) return;

        UIWindow *keyWindow = [UIApplication sharedApplication].keyWindow;
        if (!keyWindow) return;

        ESPRenderer *renderer = [ESPRenderer sharedInstance];
        [renderer renderOnView:keyWindow];
        [renderer clearDrawings];
        
        // FOV circle
        [[FOVCircleManager shared] updateOnView:keyWindow];

        UIBezierPath *combinedPath = [UIBezierPath bezierPath];
        Vector3 localPos = getPosition(local_player);
        int drawnCount = 0;
        int totalEnemies = 0;

        // ── Cache screen size (مرة وحدة بدل كل فريم) ──
        static CGFloat sW = 0, sH = 0;
        if (sW == 0) {
            CGRect b = [UIScreen mainScreen].nativeBounds;
            CGFloat scale = [UIScreen mainScreen].nativeScale;
            sW = b.size.width / scale;
            sH = b.size.height / scale;
            if (sW < sH) { CGFloat t = sW; sW = sH; sH = t; }
        }

        SimpleVec2 lineStart(sW / 2.0f, 0);  // من فوق بدل تحت

        for (int u = 0; u < players->getSize(); u++) {
            void *enemy = players->getItems()[u];
            if (!enemy || enemy == local_player) continue;
            
            // Safety checks for enemy components
            if (!game_sdk->Component_GetTransform(enemy)) continue;
            if (game_sdk->get_IsDieing(enemy) || game_sdk->get_isLocalTeam(enemy)) continue;

            totalEnemies++;
            if (drawnCount >= 30) continue;

            Vector3 pos = getPosition(enemy);
            float distance = Vector3::Distance(pos, localPos);
            if (distance > 150.0f) continue;

            SimpleVec2 bot_pos = Camera$$WorldToScreen::Regular(pos);
            if (bot_pos.x == 0 && bot_pos.y == 0) continue;

            SimpleVec2 top_pos = Camera$$WorldToScreen::Regular(pos + Vector3(0, 1.8f, 0));
            float height = fabsf(bot_pos.y - top_pos.y);
            float width = height / 2.0f;

            if (Vars.Box) {
                [renderer drawBoxFrom:SimpleVec2(bot_pos.x - width/2, top_pos.y) to:SimpleVec2(bot_pos.x + width/2, bot_pos.y) path:combinedPath];
            }

            if (Vars.lines) {
                [renderer drawLineFrom:lineStart to:SimpleVec2(bot_pos.x, top_pos.y) path:combinedPath];
            }

            if (Vars.skeleton) {
                Vector3 bones[] = {
                    GetBonePosition(enemy, game_sdk->_GetHeadPositions),
                    GetBonePosition(enemy, game_sdk->_newHipMods),
                    GetBonePosition(enemy, game_sdk->_getLeftHandTF),
                    GetBonePosition(enemy, game_sdk->_getRightHandTF),
                    GetBonePosition(enemy, game_sdk->_GetLeftAnkleTF),
                    GetBonePosition(enemy, game_sdk->_GetRightAnkleTF)
                };
                SimpleVec2 sBones[6];
                for(int i=0; i<6; i++) sBones[i] = Camera$$WorldToScreen::Regular(bones[i]);
                
                if (sBones[0].x != 0) {
                    [renderer drawLineFrom:sBones[0] to:sBones[1] path:combinedPath]; // Head to Hip
                    [renderer drawLineFrom:sBones[1] to:sBones[4] path:combinedPath]; // Hip to LAnkle
                    [renderer drawLineFrom:sBones[1] to:sBones[5] path:combinedPath]; // Hip to RAnkle
                    [renderer drawLineFrom:sBones[0] to:sBones[2] path:combinedPath]; // Head to LHand
                    [renderer drawLineFrom:sBones[0] to:sBones[3] path:combinedPath]; // Head to RHand
                }
            }

            if (Vars.Health) {
                int hpVal = game_sdk->GetHp(enemy);
                int maxHpVal = game_sdk->get_MaxHP(enemy);
                if (maxHpVal > 0) {
                    float hp = (float)hpVal / (float)maxHpVal;
                    if (hp > 1.0f) hp = 1.0f;
                    if (hp < 0.0f) hp = 0.0f;
                    [renderer drawHealthBarAt:SimpleVec2(bot_pos.x - width/2 - 4, top_pos.y) to:SimpleVec2(bot_pos.x - width/2 - 4, bot_pos.y) multiplier:hp];
                }
            }

            if (Vars.Name) {
                monoString *pname = game_sdk->name(enemy);
                if (pname) {
                    NSString *nsName = pname->toNSString();
                    if (nsName) [renderer drawTextAt:SimpleVec2(bot_pos.x, top_pos.y - 14) text:nsName];
                }
            }

            if (Vars.Distance) {
                [renderer drawTextAt:SimpleVec2(bot_pos.x + width/2 + 5, top_pos.y) text:[NSString stringWithFormat:@"%.0fm", distance]];
            }

            drawnCount++;
        }

        // Enemies counter - يبين دائماً
        [renderer drawTextAt:SimpleVec2(sW / 2, 38) text:[NSString stringWithFormat:@"[%d]", totalEnemies]];

        renderer.espLayer.path = combinedPath.CGPath;
    });
}
