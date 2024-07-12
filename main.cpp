#include <SFML/Graphics.hpp>
#include <iostream>
#include <vector>
#include <random>
#include <string>
#include <cmath>
#include <chrono>
using namespace std::chrono;


enum GameState {
    MAIN_MENU,
    GAME_PLAY,
    GAME_OVER
};

enum PowerUpType {
    SUPER_JUMP,
    SPEED_BOOST,
    MAGNET
};

class Button {
public:
    sf::Text text;
    sf::RectangleShape rectangle;

    Button(const std::string& label, sf::Font& font, unsigned int fontSize, sf::Vector2f position, sf::Vector2f size) {
        text.setFont(font);
        text.setString(label);
        text.setCharacterSize(fontSize);
        text.setFillColor(sf::Color::White);
        text.setPosition(position.x + (size.x - text.getLocalBounds().width) / 2,
            position.y + (size.y - text.getLocalBounds().height) / 2 - 10); // Center text

        rectangle.setSize(size);
        rectangle.setFillColor(sf::Color(128, 128, 128)); // Gray color
        rectangle.setPosition(position);
    }

    void drawTo(sf::RenderWindow& window) {
        window.draw(rectangle);
        window.draw(text);
    }

    bool isMouseOver(sf::RenderWindow& window) {
        float mouseX = sf::Mouse::getPosition(window).x;
        float mouseY = sf::Mouse::getPosition(window).y;
        return rectangle.getGlobalBounds().contains(mouseX, mouseY);
    }
};

class GameStats {
public:
    int score;           // Current score
    int highScore;       // High score during the session
    int saucersJumped;   // Count of saucers the bunny has jumped on

    GameStats() : score(0), highScore(0), saucersJumped(0) {}

    void addScore(int points) {
        score += points;
        if (score > highScore) {
            highScore = score;  // Update high score if current score is greater
        }
    }

    void jumpedSaucer() {
        saucersJumped++;
        addScore(10);  // Adds 10 points for every saucer jumped
    }

    void reset() {
        score = 0;
        saucersJumped = 0;
    }
};


class Bunny {
public:
    sf::Sprite sprite;
    sf::Texture texture;
    sf::Vector2f velocity;
    bool superJumpActive;
    float superJumpTimer;
    bool speedBoostActive;
    float speedBoostTimer;
    bool magnetActive;
    float magnetTimer;
    bool onSaucerLastFrame;


    Bunny(float x, float y) : superJumpActive(false), superJumpTimer(0),
        speedBoostActive(false), speedBoostTimer(0),
        magnetActive(false), magnetTimer(0),
        onSaucerLastFrame(false) { 
        if (!texture.loadFromFile("bunny.png")) {
            std::cerr << "Failed to load bunny.png" << std::endl;
        }
        sprite.setTexture(texture);
        sprite.setPosition(x, y);
        sprite.setScale(0.1f, 0.1f);
        velocity = sf::Vector2f(0, 0);
    }
    void update(bool onSaucer, float deltaTime) {
        const float gravity = 980.0f; // Increased gravity for more realistic physics
        velocity.y += gravity * deltaTime; // Apply gravity to vertical velocity

        // Check for power-ups effects
        if (superJumpActive && superJumpTimer > 0) {
            superJumpTimer -= deltaTime;
            if (superJumpTimer <= 0) superJumpActive = false;
        }

        if (speedBoostActive && speedBoostTimer > 0) {
            speedBoostTimer -= deltaTime;
            if (speedBoostTimer <= 0) {
                speedBoostActive = false;
                velocity.x /= 0.5; // Reset speed back to normal when boost ends
            }
        }

        if (magnetActive && magnetTimer > 0) {
            magnetTimer -= deltaTime;
            if (magnetTimer <= 0) magnetActive = false;
        }

        if (onSaucer && sf::Keyboard::isKeyPressed(sf::Keyboard::Space)) {
            jump();
        }

        // Adjust horizontal velocity based on current state of speed boost and keyboard input
        float baseSpeed = 300.f; // Base speed
        float currentSpeed = speedBoostActive ? baseSpeed * 2.0f : baseSpeed; // Apply boost

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::A)) {
            velocity.x = -currentSpeed;
        }
        else if (sf::Keyboard::isKeyPressed(sf::Keyboard::D)) {
            velocity.x = currentSpeed;
        }
        else {
            velocity.x = 0; // No horizontal input means no horizontal movement
        }

        // Apply the calculated velocity to update position
        sprite.move(velocity.x * deltaTime, velocity.y * deltaTime); // Correctly apply deltaTime here
    }

    void jump() {
        if (superJumpActive) {
            velocity.y = -900.f; // Increased jump height for super jump
        }
        else {
            velocity.y = -600.f; // Normal jump height
        }
    }


    void drawTo(sf::RenderWindow& window) {
        window.draw(sprite);
    }
};
class PowerUp {
public:
    sf::RectangleShape shape;
    PowerUpType type;
    bool isActive;

    PowerUp(PowerUpType type, float x, float y) : type(type), isActive(true) {
        shape.setSize(sf::Vector2f(30, 30)); // Size of the power-up
        shape.setFillColor(sf::Color::Blue); // Blue color for visibility
        shape.setPosition(x, y);
        shape.setOrigin(15, 15); // Set origin for rotation
    }

    void update(float deltaTime) {
        shape.rotate(90 * deltaTime); // Rotating effect, 90 degrees per second
    }

    void drawTo(sf::RenderWindow& window) {
        if (isActive) {
            window.draw(shape);
        }
    }

    void activate(Bunny& bunny, std::string& currentPowerUpName, float& powerUpMessageTimer) {
        isActive = false; // Mark as consumed
        switch (type) {
        case SUPER_JUMP:
            bunny.superJumpActive = true;
            bunny.superJumpTimer = 5.0; // Active for 5 seconds
            currentPowerUpName = "Super Jump";
            std::cout << "Activated Super Jump\n";
            break;
        case SPEED_BOOST:
            bunny.speedBoostActive = true;
            bunny.velocity.x *= 10.5;
            bunny.speedBoostTimer = 5.0;
            currentPowerUpName = "Speed Boost";
            std::cout << "Activated Speed Boost\n";
            break;
        case MAGNET:
            bunny.magnetActive = true;
            bunny.magnetTimer = 5.0;
            currentPowerUpName = "Magnet";
            std::cout << "Activated Magnet\n";
            // Implement magnet logic if applicable
            break;
        }
        powerUpMessageTimer = 2.0; // Display message for 2 seconds
    }
};
class FlyingSaucer {
public:
    sf::RectangleShape shape;
    float speed;
    bool hasPowerUp;
    std::vector<PowerUp> powerUps;

    FlyingSaucer(float x, float y, float width, float height) : speed(-0.5), hasPowerUp(false) {
        shape.setPosition(x, y);
        shape.setSize(sf::Vector2f(width, height));
        shape.setFillColor(sf::Color::Black);
    }

    void update(float deltaTime) {
        float actualSpeed = speed * deltaTime; // Adjust speed by deltaTime
        shape.move(actualSpeed, 0); // Move saucer horizontally

        // Update power-up positions to move with the saucer
        for (auto& powerUp : powerUps) {
            powerUp.shape.setPosition(shape.getPosition().x + shape.getSize().x / 2 - 15, shape.getPosition().y - 30);
            powerUp.update(deltaTime); // Update rotation with deltaTime
        }

        // Reverse direction at screen boundaries
        if (shape.getPosition().x + shape.getSize().x < 0 && speed < 0) {
            speed = -speed;
            shape.setPosition(0, shape.getPosition().y);
        }
        else if (shape.getPosition().x > 800 && speed > 0) {
            speed = -speed;
            shape.setPosition(800 - shape.getSize().x, shape.getPosition().y);
        }
    }


    void drawTo(sf::RenderWindow& window) {
        window.draw(shape);
        for (auto& powerUp : powerUps) {
            powerUp.drawTo(window);
        }
    }

    void spawnPowerUpOnSaucer() {
        if (!hasPowerUp) { // Only spawn a power-up if there isn't already one
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<> dis(0, 2); // Random type from 0 to 2
            PowerUpType randomType = static_cast<PowerUpType>(dis(gen));
            float x = shape.getPosition().x + shape.getSize().x / 2 - 15; // Centered on the saucer
            float y = shape.getPosition().y - 30; // Above the saucer
            powerUps.emplace_back(randomType, x, y);
            hasPowerUp = true;
        }
    }
};
class Coin {
public:
    sf::CircleShape shape;  // Using CircleShape for a coin appearance
    bool isActive;

    Coin(float x, float y) : isActive(true) {
        shape.setRadius(15); // Size of the coin
        shape.setFillColor(sf::Color::Yellow); // Coin color
        shape.setPosition(x, y);
    }

    void drawTo(sf::RenderWindow& window) {
        if (isActive) {
            window.draw(shape);
        }
    }

    bool checkCollision(const sf::FloatRect& bunnyBounds) {
        return shape.getGlobalBounds().intersects(bunnyBounds);
    }
};

int main() {
    sf::RenderWindow window(sf::VideoMode(800, 600), "Bunny Jumper", sf::Style::Close);
    window.clear(sf::Color::White); // Set background to white

    sf::Clock clock;

    sf::Font font;
    if (!font.loadFromFile("pixel-font.ttf")) {
        std::cout << "Could not load font\n";
        return -1;
    }

    sf::Text title("Bunny Jumper", font, 50);
    title.setFillColor(sf::Color::Black); // Black color
    title.setPosition(275, 50);

    Bunny bunny(375, 300);

    std::vector<Coin> coins; // Move coins vector to be accessible in the entire main function
    auto lastCoinSpawnTime = std::chrono::steady_clock::now(); // Initialize the last coin spawn time

    Button playButton("Play", font, 30, sf::Vector2f(300, 200), sf::Vector2f(200, 50));
    Button exitButton("Exit", font, 30, sf::Vector2f(300, 300), sf::Vector2f(200, 50));
    Button shopButton("Shop", font, 30, sf::Vector2f(300, 400), sf::Vector2f(200, 50));

    sf::Text gameOverText("Game Over", font, 50);
    gameOverText.setFillColor(sf::Color::Black);
    gameOverText.setPosition(300, 250); // Center it according to your window dimensions

    Button retryButton("Try Again", font, 30, sf::Vector2f(300, 350), sf::Vector2f(200, 50));

    sf::Text scoreText;
    scoreText.setFont(font);
    scoreText.setCharacterSize(24);
    scoreText.setFillColor(sf::Color::Black);
    scoreText.setPosition(10, 10);  // Position at top-left corner of the window

    sf::Text highScoreText;
    highScoreText.setFont(font);
    highScoreText.setCharacterSize(24);
    highScoreText.setFillColor(sf::Color::Black);
    highScoreText.setPosition(650, 10);  // Position at top-right corner of the window

    sf::Text powerUpMessage;
    float powerUpMessageTimer = 0;
    std::string currentPowerUpName = "";

    // Setup in the initialization section
    powerUpMessage.setFont(font);
    powerUpMessage.setCharacterSize(24);
    powerUpMessage.setFillColor(sf::Color::White);
    powerUpMessage.setPosition(400, 300); // Center on screen

    sf::Text heightText;
    heightText.setFont(font);
    heightText.setCharacterSize(24);
    heightText.setFillColor(sf::Color::Black);

    GameStats stats;

    std::vector<FlyingSaucer> saucers;
    // Initial saucer positioning
    saucers.push_back(FlyingSaucer(200, 500, 100, 20));
    saucers.push_back(FlyingSaucer(500, 350, 100, 20));
    saucers.push_back(FlyingSaucer(300, 200, 100, 20));


    // Variables to track saucer spawning
    float minHeight = 600; // Track the minimum height (highest point) the bunny has reached
    float spawnHeight = 200; // Initial height for the first saucer above the screen view

    auto lastPowerUpSpawnTime = steady_clock::now();

    GameState currentState = MAIN_MENU;

    while (window.isOpen()) {
        auto now = steady_clock::now();
        auto timeSinceLastPowerUp = duration_cast<seconds>(now - lastPowerUpSpawnTime).count();

        if (timeSinceLastPowerUp >= 5) { // Check if 5 seconds have elapsed
            // Find the highest saucer that doesn't have a power-up
            FlyingSaucer* highestSaucer = nullptr;
            for (auto& saucer : saucers) {
                if (!saucer.hasPowerUp && (highestSaucer == nullptr || saucer.shape.getPosition().y < highestSaucer->shape.getPosition().y)) {
                    highestSaucer = &saucer;
                }
            }
            if (highestSaucer) {
                highestSaucer->spawnPowerUpOnSaucer();
                lastPowerUpSpawnTime = now; // Reset the timer
            }
        }
        sf::Time elapsed = clock.restart(); // Restart the clock and get elapsed time
        float deltaTime = elapsed.asSeconds();

        scoreText.setString("Score: " + std::to_string(stats.score));
        highScoreText.setString("High Score: " + std::to_string(stats.highScore));
        window.draw(scoreText);
        window.draw(highScoreText);

        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();

            if (event.type == sf::Event::MouseButtonPressed) {
                if (currentState == MAIN_MENU) {
                    if (playButton.isMouseOver(window)) {
                        currentState = GAME_PLAY; // Change state to gameplay
                    }
                    if (currentState == GAME_OVER) {
                        if (retryButton.isMouseOver(window)) {
                            // Reset everything and start new game
                            currentState = GAME_PLAY;
                        }
                    }
                    if (exitButton.isMouseOver(window)) {
                        window.close();
                    }
                    if (shopButton.isMouseOver(window)) {
                        std::cout << "Open Shop\n"; // Placeholder for shop
                    }
                }
            }
        }
        // Random device for saucer spawning
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<> dis(100, 700); // Saucer spawning positions


        float lastY = 300; // Initial position for dynamic spawning

        // Calculate height based on some reference point, e.g., initial bunny position or screen bottom
        float currentHeight = window.getSize().y - bunny.sprite.getPosition().y;  // Assuming (0,0) is at the top

        // Set the string for the height text
        heightText.setString("Height: " + std::to_string(static_cast<int>(currentHeight)) + " units");
        if (currentHeight <= 0.0f) {
            currentState = GAME_OVER;
            std::cout << "Game Over: below 0 height" << std::endl;
        }
        window.clear(sf::Color::White);
        if (currentState == GAME_OVER) {
            window.draw(gameOverText);
            retryButton.drawTo(window);

            if (event.type == sf::Event::MouseButtonPressed) {
                if (retryButton.isMouseOver(window)) {
                    // Reset game stats and positions
                    stats.reset();
                    bunny = Bunny(375, 300); // Reset bunny position
                    currentState = GAME_PLAY;
                }
            }
        }

        else if (currentState == MAIN_MENU) {
            window.draw(title);
            playButton.drawTo(window);
            exitButton.drawTo(window);
            shopButton.drawTo(window);
        }
        else if (currentState == GAME_PLAY) {
            bool onSaucer = false;
            // Minimum height calculation needs resetting every frame
            float currentMinHeight = minHeight;

            // Coin spawning logic
            auto timeSinceLastCoin = std::chrono::duration_cast<std::chrono::seconds>(now - lastCoinSpawnTime).count();
            if (timeSinceLastCoin >= 10) { // Spawn coin every 10 seconds
                std::uniform_real_distribution<> disX(0, window.getSize().x - 30); // Adjust to prevent spawn outside the view
                float newY = bunny.sprite.getPosition().y - 200; // Coins spawn above the bunny
                coins.emplace_back(disX(gen), newY);
                lastCoinSpawnTime = now; // Reset the timer after spawning a coin
            }

            // Update and draw coins
            for (auto& coin : coins) {
                if (coin.isActive && coin.checkCollision(bunny.sprite.getGlobalBounds())) {
                    coin.isActive = false; // Coin collected
                    stats.score += 200; // Add points for collecting a coin
                }
                coin.drawTo(window);
            }

            // Clean up inactive coins to free memory
            coins.erase(std::remove_if(coins.begin(), coins.end(), [](const Coin& coin) {
                return !coin.isActive;
                }), coins.end());

            for (auto& saucer : saucers) {
                saucer.update(0.5);
                saucer.drawTo(window);
                bool isOnCurrentSaucer = bunny.sprite.getGlobalBounds().intersects(saucer.shape.getGlobalBounds()) && bunny.velocity.y >= 0;
                if (isOnCurrentSaucer) {
                    bunny.velocity.y = 0;
                    bunny.sprite.setPosition(bunny.sprite.getPosition().x, saucer.shape.getPosition().y - bunny.sprite.getGlobalBounds().height);
                    if (!bunny.onSaucerLastFrame) {
                        stats.jumpedSaucer(); // Only call this when first landing on a saucer
                    }
                    onSaucer = true;

                    currentMinHeight = std::min(currentMinHeight, saucer.shape.getPosition().y);
                }
                for (auto& powerUp : saucer.powerUps) {
                    powerUp.update(deltaTime);
                    if (powerUp.isActive && powerUp.shape.getGlobalBounds().intersects(bunny.sprite.getGlobalBounds())) {
                        powerUp.activate(bunny, currentPowerUpName, powerUpMessageTimer);
                    }
                }

            }
            // After all saucers have been processed, update the onSaucerLastFrame
            bunny.onSaucerLastFrame = onSaucer;

            // If the bunny is no longer on any saucer, reset the onSaucer flag
            if (!onSaucer) {
                bunny.onSaucerLastFrame = false;
            }

            if (powerUpMessageTimer > 0) {
                powerUpMessageTimer -= deltaTime;
                powerUpMessage.setString(currentPowerUpName + " Activated!");
            }
            else {
                powerUpMessage.setString("");

            }

            bunny.update(onSaucer, deltaTime);
            bunny.drawTo(window);

            if (currentMinHeight < minHeight) {
                minHeight = currentMinHeight;
                // Spawn new saucers progressively higher as the bunny ascends
                spawnHeight = minHeight - 100; // Adjust spawn height based on the new minHeight
                saucers.push_back(FlyingSaucer(dis(gen), spawnHeight, 100, 20));
            }

            // Remove off-screen saucers
            saucers.erase(std::remove_if(saucers.begin(), saucers.end(), [&](const FlyingSaucer& s) {
                return s.shape.getPosition().y > window.getSize().y;
                }), saucers.end());

            // Adjust the view to follow the bunny if it moves up
            if (bunny.sprite.getPosition().y < 300) {
                window.setView(sf::View(sf::FloatRect(0, bunny.sprite.getPosition().y - 300, 800, 600)));
            }
            // Update score and high score positions according to the view
            sf::View currentView = window.getView();
            scoreText.setPosition(currentView.getCenter().x - 390, currentView.getCenter().y - 290);
            highScoreText.setPosition(currentView.getCenter().x + 250, currentView.getCenter().y - 290);

            // Update power-up message position and text according to the view
            if (powerUpMessageTimer > 0) {
                powerUpMessageTimer -= deltaTime;
                powerUpMessage.setString(currentPowerUpName + " Activated!");
                powerUpMessage.setPosition(currentView.getCenter().x - powerUpMessage.getLocalBounds().width / 2, currentView.getCenter().y - 300); // Center at the top of the screen
            }
            else {
                powerUpMessage.setString(""); // Clear the message when the timer is done
            }

            // Ensure to draw the power-up message last so it appears over other elements
            window.draw(powerUpMessage);
        }
        // Update score display based on current view
        sf::View currentView = window.getView();
        scoreText.setPosition(currentView.getCenter().x - 390, currentView.getCenter().y - 290);
        highScoreText.setPosition(currentView.getCenter().x + 150, currentView.getCenter().y - 290);
        heightText.setPosition(currentView.getCenter().x - 390, currentView.getCenter().y - 250);

        scoreText.setString("Score: " + std::to_string(stats.score));
        highScoreText.setString("High Score: " + std::to_string(stats.highScore));
        window.draw(scoreText);
        window.draw(highScoreText);
        window.draw(heightText);

        // Reset view when going back to main menu
        if (currentState == MAIN_MENU) {
            window.setView(sf::View(sf::FloatRect(0, 0, 800, 600)));
        }

        window.display();
    }

    return 0;
}
