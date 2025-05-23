#include <vector>
#include <algorithm>
#include <functional> 
#include <memory>
#include <cstdlib>
#include <cmath>
#include <ctime>

#include <raylib.h>
#include <raymath.h>
#include <array>
#include <optional>

// --- UTILS ---
namespace Utils {
	inline static float RandomFloat(float min, float max) {
		return min + static_cast<float>(rand()) / RAND_MAX * (max - min);
	}
}

// --- TRANSFORM, PHYSICS, LIFETIME, RENDERABLE ---
struct TransformA {
	Vector2 position{};
	float rotation{};
};

struct Physics {
	Vector2 velocity{};
	float rotationSpeed{};
};

struct Renderable {
	enum Size { SMALL = 1, MEDIUM = 2, LARGE = 4 } size = SMALL;
};

// --- RENDERER ---
class Renderer {
public:
	static Renderer& Instance() {
		static Renderer inst;
		return inst;
	}

	void Init(int w, int h, const char* title) {
		InitWindow(w, h, title);
		SetTargetFPS(60);
		screenW = w;
		screenH = h;
		backgroundTexture = LoadTexture("backk.png");
	}

	void Begin() {
		BeginDrawing();
		ClearBackground(BLACK);  // Dodane czyszczenie tła, aby uniknąć "rozmazywania"
		DrawTexturePro(
			backgroundTexture,
			{ 0, 0, (float)backgroundTexture.width, (float)backgroundTexture.height },
			{ 0, 0, (float)screenW, (float)screenH },
			{ 0, 0 },
			0.0f,
			WHITE
		);
	}

	void End() {
		EndDrawing();
	}

	void DrawPoly(const Vector2& pos, int sides, float radius, float rot) {
		DrawPolyLines(pos, sides, radius, rot, WHITE);
	}

	int Width() const {
		return screenW;
	}

	int Height() const {
		return screenH;
	}

private:
	Renderer() = default;

	int screenW{};
	int screenH{};
	Texture2D backgroundTexture;
};


// --- ASTEROID HIERARCHY ---

class Asteroid {
public:
	Asteroid(int screenW, int screenH) {
		init(screenW, screenH);
	}
	virtual ~Asteroid() = default;
	virtual std::optional<std::array<Vector2, 4>> GetOBB() const {
    return std::nullopt; // domyślnie brak OBB dla zwykłych asteroid
}

	

	bool Update(float dt) {
		transform.position = Vector2Add(transform.position, Vector2Scale(physics.velocity, dt));
		transform.rotation += physics.rotationSpeed * dt;
		if (transform.position.x < -GetRadius() || transform.position.x > Renderer::Instance().Width() + GetRadius() ||
			transform.position.y < -GetRadius() || transform.position.y > Renderer::Instance().Height() + GetRadius())
			return false;
		return true;
	}
	virtual void Draw() const = 0;

	Vector2 GetPosition() const {
		return transform.position;
	}

	float constexpr GetRadius() const {
		return 16.f * (float)render.size;
	}

	int GetDamage() const {
		return baseDamage * static_cast<int>(render.size);
	}

	int GetSize() const {
		return static_cast<int>(render.size);
	}

	
protected:
	void init(int screenW, int screenH) {
		// Choose size
		//render.size = static_cast<Renderable::Size>(1 << GetRandomValue(0, 2));

		// Spawn at random edge
		switch (GetRandomValue(0, 3)) {
		case 0:
			transform.position = { Utils::RandomFloat(0, screenW), -GetRadius() };
			break;
		case 1:
			transform.position = { screenW + GetRadius(), Utils::RandomFloat(0, screenH) };
			break;
		case 2:
			transform.position = { Utils::RandomFloat(0, screenW), screenH + GetRadius() };
			break;
		default:
			transform.position = { -GetRadius(), Utils::RandomFloat(0, screenH) };
			break;
		}

		// Aim towards center with jitter
		float maxOff = fminf(screenW, screenH) * 0.1f;
		float ang = Utils::RandomFloat(0, 2 * PI);
		float rad = Utils::RandomFloat(0, maxOff);
		Vector2 center = {
										 screenW * 0.5f + cosf(ang) * rad,
										 screenH * 0.5f + sinf(ang) * rad
		};

		Vector2 dir = Vector2Normalize(Vector2Subtract(center, transform.position));
		physics.velocity = Vector2Scale(dir, Utils::RandomFloat(SPEED_MIN, SPEED_MAX));
		physics.rotationSpeed = Utils::RandomFloat(ROT_MIN, ROT_MAX);

		transform.rotation = Utils::RandomFloat(0, 360);
	}

	TransformA transform;
	Physics    physics;
	Renderable render;

	int baseDamage = 0;
	static constexpr float LIFE = 10.f;
	static constexpr float SPEED_MIN = 125.f;
	static constexpr float SPEED_MAX = 250.f;
	static constexpr float ROT_MIN = 50.f;
	static constexpr float ROT_MAX = 240.f;
};

class TriangleAsteroid : public Asteroid {

public:
	TriangleAsteroid(int w, int h) : Asteroid(w, h) {
	baseDamage = 5;
	texture = LoadTexture("vgg.png");
	GenTextureMipmaps(&texture);
	SetTextureFilter(texture, TEXTURE_FILTER_BILINEAR);
	scale = 0.2f;	
}
~TriangleAsteroid() override {
		UnloadTexture(texture);
	}
	void Draw() const override {
    Rectangle source = { 0, 0, (float)texture.width, (float)texture.height };
    float w = texture.width * scale;
    float h = texture.height * scale;

    Rectangle dest = { transform.position.x, transform.position.y, w, h };
    Vector2 origin = { w * 0.5f, h * 0.5f }; // <<< bardzo ważne

    DrawTexturePro(texture, source, dest, origin, transform.rotation, WHITE);

     //DEBUG: Rysuj OBB
    //auto obb = GetOBB();
    //if (obb.has_value()) {
        //for (int i = 0; i < 4; ++i) {
           // DrawLineV(obb->at(i), obb->at((i + 1) % 4), GREEN);
        //}
    //}
}
	private:
	mutable Texture2D texture;
	float scale;

	public:
	std::optional<std::array<Vector2, 4>> GetOBB() const override {
    Vector2 center = transform.position;
    float scaleFactor = 0.8f; // <--- tu regulujesz "mniejszość"
    float w = texture.width * scale * scaleFactor;
    float h = texture.height * scale * scaleFactor;
    float angleRad = DEG2RAD * transform.rotation;

    Vector2 halfExtents = { w * 0.5f, h * 0.5f };
    Vector2 corners[4] = {
        {-halfExtents.x, -halfExtents.y},
        { halfExtents.x, -halfExtents.y},
        { halfExtents.x,  halfExtents.y},
        {-halfExtents.x,  halfExtents.y}
    };

    std::array<Vector2, 4> obbCorners;
    for (int i = 0; i < 4; ++i) {
        obbCorners[i] = Vector2Rotate(corners[i], angleRad);
        obbCorners[i] = Vector2Add(obbCorners[i], center);
    }

    return obbCorners;
}
};
class SquareAsteroid : public Asteroid {
public:
	SquareAsteroid(int w, int h) : Asteroid(w, h) { baseDamage = 10; }
	void Draw() const override {
		Renderer::Instance().DrawPoly(transform.position, 4, GetRadius(), transform.rotation);
	}
};
class PentagonAsteroid : public Asteroid {
public:
	PentagonAsteroid(int w, int h) : Asteroid(w, h) { baseDamage = 15; }
	void Draw() const override {
		Renderer::Instance().DrawPoly(transform.position, 5, GetRadius(), transform.rotation);
	}
};

// Shape selector
enum class AsteroidShape { TRIANGLE = 3, SQUARE = 4, PENTAGON = 5, RANDOM = 0 };

// Factory
static inline std::unique_ptr<Asteroid> MakeAsteroid(int w, int h, AsteroidShape shape) {
	switch (shape) {
	case AsteroidShape::TRIANGLE:
		return std::make_unique<TriangleAsteroid>(w, h);
	case AsteroidShape::SQUARE:
		return std::make_unique<SquareAsteroid>(w, h);
	case AsteroidShape::PENTAGON:
		return std::make_unique<PentagonAsteroid>(w, h);
	default: {
		return MakeAsteroid(w, h, static_cast<AsteroidShape>(3 + GetRandomValue(0, 2)));
	}
	}
}

// --- PROJECTILE HIERARCHY ---
enum class WeaponType { LASER, BULLET, ROCKET, COUNT };
class Projectile {
public:
	Projectile(Vector2 pos, Vector2 vel, int dmg, WeaponType wt)
	{
		transform.position = pos;
		physics.velocity = vel;
		baseDamage = dmg;
		type = wt;
	}
	bool Update(float dt) {
		transform.position = Vector2Add(transform.position, Vector2Scale(physics.velocity, dt));

		if (transform.position.x < 0 ||
			transform.position.x > Renderer::Instance().Width() ||
			transform.position.y < 0 ||
			transform.position.y > Renderer::Instance().Height())
		{
			return true;
		}
		return false;
	}
	void Draw() const {
    if (type == WeaponType::BULLET) {
        DrawCircleV(transform.position, 5.f, WHITE);
    }
    else if (type == WeaponType::LASER) {
        static constexpr float LASER_LENGTH = 30.f;
        Rectangle lr = { transform.position.x - 2.f, transform.position.y - LASER_LENGTH, 4.f, LASER_LENGTH };
        DrawRectangleRec(lr, RED);
    }
    else if (type == WeaponType::ROCKET) {
        DrawCircleV(transform.position, 8.f, ORANGE);
    }
}
	Vector2 GetPosition() const {
		return transform.position;
	}

	float GetRadius() const {
		return (type == WeaponType::BULLET) ? 5.f : 2.f;
	}

	int GetDamage() const {
		return baseDamage;
	}

private:
	TransformA transform;
	Physics    physics;
	int        baseDamage;
	WeaponType type;
};

inline static Projectile MakeProjectile(WeaponType wt,
	const Vector2 pos,
	float speed)
{
	Vector2 vel{ 0, -speed };
	if (wt == WeaponType::LASER) {
		return Projectile(pos, vel, 20, wt);
	}
	else if (wt == WeaponType::BULLET) {
		return Projectile(pos, vel, 10, wt);
	}
	else if (wt == WeaponType::ROCKET) {
		return Projectile(pos, vel, 40, wt); // mocniejszy
	}

	// Fallback - domyślny pocisk, by uniknąć ostrzeżenia/bledu
	return Projectile(pos, vel, 5, WeaponType::BULLET);
}

// --- SHIP HIERARCHY ---
class Ship {
public:
	Ship(int screenW, int screenH) {
		transform.position = {
												 screenW * 0.5f,
												 screenH * 0.5f
		};
		hp = 100;
		speed = 250.f;
		alive = true;

		// per-weapon fire rate & spacing
		fireRateLaser = 18.f; // shots/sec
		fireRateBullet = 22.f;
		spacingLaser = 40.f; // px between lasers
		spacingBullet = 20.f;
	}
	virtual ~Ship() = default;
	virtual void Update(float dt) = 0;
	virtual void Draw() const = 0;

	void TakeDamage(int dmg) {
		if (!alive) return;
		hp -= dmg;
		if (hp <= 0) alive = false;
	}

	bool IsAlive() const {
		return alive;
	}

	Vector2 GetPosition() const {
		return transform.position;
	}

	virtual float GetRadius() const = 0;

	int GetHP() const {
		return hp;
	}

	float GetFireRate(WeaponType wt) const {
		return (wt == WeaponType::LASER) ? fireRateLaser : fireRateBullet;
	}

	float GetSpacing(WeaponType wt) const {
		return (wt == WeaponType::LASER) ? spacingLaser : spacingBullet;
	}

protected:
	TransformA transform;
	int        hp;
	float      speed;
	bool       alive;
	float      fireRateLaser;
	float      fireRateBullet;
	float      spacingLaser;
	float      spacingBullet;
};

class PlayerShip :public Ship {
public:
	PlayerShip(int w, int h) : Ship(w, h) {
		texture = LoadTexture("spaceship2.png");
		GenTextureMipmaps(&texture);                                                        // Generate GPU mipmaps for a texture
		SetTextureFilter(texture, 2);
		scale = 0.02f;
	}
	~PlayerShip() {
		UnloadTexture(texture);
	}
	float tiltAngle = 0.0f;

	inline float LerpAngle(float a, float b, float t) {
    float diff = fmodf(b - a + 540.0f, 360.0f) - 180.0f;
    return a + diff * t;
}

	void Update(float dt) override {
	if (alive) {
		Vector2 move = { 0, 0 };

		if (IsKeyDown(KEY_W)) move.y -= 1;
		if (IsKeyDown(KEY_S)) move.y += 1;
		if (IsKeyDown(KEY_A)) move.x -= 1;
		if (IsKeyDown(KEY_D)) move.x += 1;

		move = Vector2Normalize(move);
		transform.position.x += move.x * speed * dt;
		transform.position.y += move.y * speed * dt;

		// Oblicz docelowy przechył (tylko na podstawie ruchu w poziomie)
		float targetTilt = 0.0f;
		if (IsKeyDown(KEY_A)) targetTilt = -10.0f;
		else if (IsKeyDown(KEY_D)) targetTilt = 10.0f;

		// Wygładzanie przechyłu
		float smoothing = 8.0f * dt;
		tiltAngle += (targetTilt - tiltAngle) * smoothing;
	}
	else {
		transform.position.y += speed * dt;
		tiltAngle = 0.0f;  // brak przechyłu po śmierci
	}
}

	void Draw() const override {
		if (!alive && fmodf(GetTime(), 0.4f) > 0.2f) return;
		Vector2 dstPos = {
										 transform.position.x - (texture.width * scale) * 0.5f,
										 transform.position.y - (texture.height * scale) * 0.5f
		};
		DrawTextureEx(texture, dstPos, tiltAngle, scale, WHITE);
	}

	float GetRadius() const override {
		return (texture.width * scale) * 0.5f;
	}

private:
	Texture2D texture;
	float     scale;
};


bool PointInOBB(Vector2 point, const std::array<Vector2, 4>& obb) {
    for (int i = 0; i < 4; ++i) {
        Vector2 p1 = obb[i];
        Vector2 p2 = obb[(i + 1) % 4];
        Vector2 edge = Vector2Subtract(p2, p1);
        Vector2 normal = { -edge.y, edge.x };

        Vector2 toPoint = Vector2Subtract(point, p1);
        if (Vector2DotProduct(normal, toPoint) > 0) {
            return false;
        }
    }
    return true;
}

bool RectOverlapsOBB(Rectangle rect, const std::array<Vector2, 4>& obb) {
    Vector2 corners[4] = {
        {rect.x, rect.y},
        {rect.x + rect.width, rect.y},
        {rect.x + rect.width, rect.y + rect.height},
        {rect.x, rect.y + rect.height}
    };

    for (auto& corner : corners) {
        if (PointInOBB(corner, obb)) {
            return true;
        }
    }

    return false;
}

bool OBBvsOBB(const std::array<Vector2, 4>& a, const std::array<Vector2, 4>& b) {
    auto ProjectOntoAxis = [](const std::array<Vector2, 4>& points, Vector2 axis) {
        float min = Vector2DotProduct(points[0], axis);
        float max = min;
        for (int i = 1; i < 4; ++i) {
            float proj = Vector2DotProduct(points[i], axis);
            if (proj < min) min = proj;
            if (proj > max) max = proj;
        }
        return std::make_pair(min, max);
    };

    auto GetAxes = [](const std::array<Vector2, 4>& points) {
        std::array<Vector2, 2> axes;
        axes[0] = Vector2Normalize(Vector2Subtract(points[1], points[0]));
        axes[1] = Vector2Normalize(Vector2Subtract(points[3], points[0]));
        axes[0] = { -axes[0].y, axes[0].x };
        axes[1] = { -axes[1].y, axes[1].x };
        return axes;
    };

    auto axesA = GetAxes(a);
    auto axesB = GetAxes(b);

    for (const auto& axis : {axesA[0], axesA[1], axesB[0], axesB[1]}) {
        auto [minA, maxA] = ProjectOntoAxis(a, axis);
        auto [minB, maxB] = ProjectOntoAxis(b, axis);
        if (maxA < minB || maxB < minA) return false;
    }
    return true;
}

// --- APPLICATION ---
class Application {
public:
	static Application& Instance() {
		static Application inst;
		return inst;
	}

	void Run() {
		srand(static_cast<unsigned>(time(nullptr)));
		Renderer::Instance().Init(C_WIDTH, C_HEIGHT, "Asteroids OOP");

		auto player = std::make_unique<PlayerShip>(C_WIDTH, C_HEIGHT);

		float spawnTimer = 0.f;
		float spawnInterval = Utils::RandomFloat(C_SPAWN_MIN, C_SPAWN_MAX);
		WeaponType currentWeapon = WeaponType::LASER;
		float shotTimer = 0.f;

		while (!WindowShouldClose()) {
			float dt = GetFrameTime();
			spawnTimer += dt;

			// Update player
			player->Update(dt);

			// Restart logic
			if (!player->IsAlive() && IsKeyPressed(KEY_R)) {
				player = std::make_unique<PlayerShip>(C_WIDTH, C_HEIGHT);
				asteroids.clear();
				projectiles.clear();
				spawnTimer = 0.f;
				spawnInterval = Utils::RandomFloat(C_SPAWN_MIN, C_SPAWN_MAX);
			}
			// Asteroid shape switch
			if (IsKeyPressed(KEY_ONE)) {
				currentShape = AsteroidShape::TRIANGLE;
			}
			if (IsKeyPressed(KEY_TWO)) {
				currentShape = AsteroidShape::SQUARE;
			}
			if (IsKeyPressed(KEY_THREE)) {
				currentShape = AsteroidShape::PENTAGON;
			}
			if (IsKeyPressed(KEY_FOUR)) {
				currentShape = AsteroidShape::RANDOM;
			}

			// Weapon switch
			if (IsKeyPressed(KEY_TAB)) {
				currentWeapon = static_cast<WeaponType>((static_cast<int>(currentWeapon) + 1) % static_cast<int>(WeaponType::COUNT));
			}

			// Shooting
			{
				if (player->IsAlive() && IsKeyDown(KEY_SPACE)) {
					shotTimer += dt;
					float interval = 1.f / player->GetFireRate(currentWeapon);
					float projSpeed = player->GetSpacing(currentWeapon) * player->GetFireRate(currentWeapon);

					while (shotTimer >= interval) {
						Vector2 p = player->GetPosition();
						p.y -= player->GetRadius();
						projectiles.push_back(MakeProjectile(currentWeapon, p, projSpeed));
						shotTimer -= interval;
					}
				}
				else {
					float maxInterval = 1.f / player->GetFireRate(currentWeapon);

					if (shotTimer > maxInterval) {
						shotTimer = fmodf(shotTimer, maxInterval);
					}
				}
			}

			// Spawn asteroids
			if (spawnTimer >= spawnInterval && asteroids.size() < MAX_AST) {
				asteroids.push_back(MakeAsteroid(C_WIDTH, C_HEIGHT, currentShape));
				spawnTimer = 0.f;
				spawnInterval = Utils::RandomFloat(C_SPAWN_MIN, C_SPAWN_MAX);
			}

			// Update projectiles - check if in boundries and move them forward
			{
				auto projectile_to_remove = std::remove_if(projectiles.begin(), projectiles.end(),
					[dt](auto& projectile) {
						return projectile.Update(dt);
					});
				projectiles.erase(projectile_to_remove, projectiles.end());
			}

			// Projectile-Asteroid collisions O(n^2)
			for (auto pit = projectiles.begin(); pit != projectiles.end();) {
				bool removed = false;

				for (auto ait = asteroids.begin(); ait != asteroids.end(); ++ait) {
					bool hit = false;

auto aOBB = (*ait)->GetOBB();
if (aOBB.has_value() && pit->GetRadius() == 2.f) { // LASER
    Rectangle laserRect = {
        pit->GetPosition().x - 2.f,
        pit->GetPosition().y - 30.f,
        4.f,
        30.f
    };
    std::array<Vector2, 4> laserOBB = {
        Vector2{laserRect.x, laserRect.y},
        Vector2{laserRect.x + laserRect.width, laserRect.y},
        Vector2{laserRect.x + laserRect.width, laserRect.y + laserRect.height},
        Vector2{laserRect.x, laserRect.y + laserRect.height}
    };
    hit = OBBvsOBB(aOBB.value(), laserOBB);
}
else {
    float dist = Vector2Distance(pit->GetPosition(), (*ait)->GetPosition());
    hit = dist < pit->GetRadius() + (*ait)->GetRadius();
}

if (hit) {
    ait = asteroids.erase(ait);
    pit = projectiles.erase(pit);
    removed = true;
    break;
}
				}
				if (!removed) {
					++pit;
				}
			}

			// Asteroid-Ship collisions
			{
				auto remove_collision =
					[&player, dt](auto& asteroid_ptr_like) -> bool {
					if (player->IsAlive()) {
						float dist = Vector2Distance(player->GetPosition(), asteroid_ptr_like->GetPosition());

						if (dist < player->GetRadius() + asteroid_ptr_like->GetRadius()) {
							player->TakeDamage(asteroid_ptr_like->GetDamage());
							return true; // Mark asteroid for removal due to collision
						}
					}
					if (!asteroid_ptr_like->Update(dt)) {
						return true;
					}
					return false; // Keep the asteroid
					};
				auto asteroid_to_remove = std::remove_if(asteroids.begin(), asteroids.end(), remove_collision);
				asteroids.erase(asteroid_to_remove, asteroids.end());
			}

			// Render everything
			{
				Renderer::Instance().Begin();

				DrawText(TextFormat("HP: %d", player->GetHP()),
					10, 10, 20, GREEN);

				const char* weaponName = "";
                switch (currentWeapon) {
	            case WeaponType::LASER: weaponName = "LASER"; break;
	            case WeaponType::BULLET: weaponName = "BULLET"; break;
	            case WeaponType::ROCKET: weaponName = "ROCKET"; break;
                }

				DrawText(TextFormat("Weapon: %s", weaponName),
					10, 40, 20, BLUE);

				for (const auto& projPtr : projectiles) {
					projPtr.Draw();
				}
				for (const auto& astPtr : asteroids) {
					astPtr->Draw();
				}

				player->Draw();

                     if (!player->IsAlive()) {
	                 DrawText("GAME OVER", C_WIDTH / 2 - MeasureText("GAME OVER", 40) / 2, C_HEIGHT / 2 - 40, 40, RED);
	                 DrawText("Press R to restart", C_WIDTH / 2 - MeasureText("Press R to restart", 20) / 2, C_HEIGHT / 2 + 10, 20, LIGHTGRAY);
                    }

				Renderer::Instance().End();
			}
		}
	}

private:
	Application()
	{
		asteroids.reserve(1000);
		projectiles.reserve(10'000);
	};

	std::vector<std::unique_ptr<Asteroid>> asteroids;
	std::vector<Projectile> projectiles;

	AsteroidShape currentShape = AsteroidShape::TRIANGLE;

	static constexpr int C_WIDTH = 800;
	static constexpr int C_HEIGHT = 800;
	static constexpr size_t MAX_AST = 150;
	static constexpr float C_SPAWN_MIN = 0.5f;
	static constexpr float C_SPAWN_MAX = 3.0f;

	static constexpr int C_MAX_ASTEROIDS = 1000;
	static constexpr int C_MAX_PROJECTILES = 10'000;
};

int main() {
	Application::Instance().Run();
	return 0;
}