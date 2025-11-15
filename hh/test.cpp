#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <vector>
#include <string>
#include <fstream>

sf::Vector2f ballVelocity(0.f, -10.f);
bool soundOn = true;
float volume = 100.f;
std::string language = "English";
std::string playerName = "Player";
int highestScore = 0;
sf::Color ballColor = sf::Color::Red;

void saveHighScore(int score) {
    std::ofstream outFile("highscore.txt");
    if (outFile.is_open()) {
        outFile << score;
        outFile.close();
    }
}

int loadHighScore() {
    std::ifstream inFile("highscore.txt");
    int score = 0;
    if (inFile.is_open()) {
        inFile >> score;
        inFile.close();
    }
    return score;
}

class Block {
public:
    sf::RectangleShape shape;
    int hitCount;
    int maxHits;

    Block(sf::Vector2f size, sf::Vector2f position, int maxHits)
        : hitCount(0), maxHits(maxHits) {
        shape.setSize(size);
        shape.setPosition(position);
        shape.setFillColor(sf::Color::Blue);
    }

    virtual void hit() {
        hitCount++;
        if (hitCount < maxHits) {
            shape.setFillColor(sf::Color(255, 255 - (hitCount * 25), 255 - (hitCount * 25)));
        } else {
            shape.setFillColor(sf::Color::Transparent);
        }
    }

    bool isDestroyed() const {
        return hitCount >= maxHits;
    }

    virtual bool decreasesLife() const {
        return false;
    }

    virtual void move() {}
};

class SpecialBlock : public Block {
public:
    SpecialBlock(sf::Vector2f size, sf::Vector2f position, int maxHits)
        : Block(size, position, maxHits) {
        shape.setFillColor(sf::Color::Red);
    }

    bool decreasesLife() const override {
        return true;
    }
};

class BoomBlock : public Block {
public:
    BoomBlock(sf::Vector2f size, sf::Vector2f position, int maxHits)
        : Block(size, position, maxHits) {
        shape.setFillColor(sf::Color::Yellow);
    }

    bool decreasesLife() const override {
        return true;
    }
};

class MovingBlock : public Block {
public:
    sf::Vector2f velocity;

    MovingBlock(sf::Vector2f size, sf::Vector2f position, int maxHits, sf::Vector2f velocity)
        : Block(size, position, maxHits), velocity(velocity) {
        shape.setFillColor(sf::Color::Green);
    }

    void move() override {
        shape.move(velocity);
        if (shape.getPosition().x < 0 || shape.getPosition().x + shape.getSize().x > 800) {
            velocity.x = -velocity.x;
        }
        if (shape.getPosition().y < 0 || shape.getPosition().y + shape.getSize().y > 600) {
            velocity.y = -velocity.y;
        }
    }
};

void autoMoveBall(std::vector<sf::CircleShape> &balls, sf::RenderWindow &window, std::vector<Block*> &blocks, sf::RectangleShape &paddle, int &lives, int &score, sf::Sound &hitSound, sf::Sound &lifeLossSound, sf::Sound &bottomHitSound) {
    for (auto &ball : balls) {
        ball.move(ballVelocity);

        // Check for collision with window borders
        if (ball.getPosition().x < 0 || ball.getPosition().x + ball.getRadius() * 2 > window.getSize().x) {
            ballVelocity.x = -ballVelocity.x;
        }
        if (ball.getPosition().y < 0) {
            ballVelocity.y = -ballVelocity.y;
        }

        // Check for collision with paddle
        if (ball.getGlobalBounds().intersects(paddle.getGlobalBounds())) {
            ballVelocity.y = -ballVelocity.y;
            float paddleCenter = paddle.getPosition().x + paddle.getSize().x / 2;
            float ballCenter = ball.getPosition().x + ball.getRadius();
            ballVelocity.x = (ballCenter - paddleCenter) / 10;
        }

        // Check for collision with blocks
        int touchCount = 0;
        for (auto it = blocks.begin(); it != blocks.end();) {
            (*it)->move(); // Move the block if it is a moving block
            if (ball.getGlobalBounds().intersects((*it)->shape.getGlobalBounds())) {
                ballVelocity.y = -ballVelocity.y;
                (*it)->hit();
                if (soundOn) hitSound.play(); // Play hit sound
                touchCount++;
                if ((*it)->isDestroyed()) {
                    if ((*it)->decreasesLife()) {
                        lives--; // Decrease the number of lives
                        if (soundOn) lifeLossSound.play(); // Play life loss sound
                    }
                    delete *it;
                    it = blocks.erase(it); // Remove the block if it is destroyed
                    score += 5; // Increase the score by 5 points
                } else {
                    ++it;
                }
            } else {
                ++it;
            }
        }

        // Split the ball if it touches more than 3 blocks simultaneously
        if (touchCount >= 3) {
            sf::CircleShape newBall(ball.getRadius());
            newBall.setPosition(ball.getPosition());
            newBall.setFillColor(ball.getFillColor());
            balls.push_back(newBall);
        }

        // Reset ball position if it goes below the paddle
        if (ball.getPosition().y > window.getSize().y) {
            ball.setPosition(window.getSize().x / 2 - ball.getRadius(), window.getSize().y - ball.getRadius() * 2 - 5);
            ballVelocity = sf::Vector2f(0.f, -10.f);
            lives--; // Decrease the number of lives
            if (soundOn) bottomHitSound.play(); // Play bottom hit sound
        }
    }
}

void initializeBlocks(std::vector<Block*> &blocks, int level) {
    blocks.clear();
    // Add default boom blocks
    for (int i = 0; i < level; ++i) {
        blocks.push_back(new BoomBlock(sf::Vector2f(50, 20), sf::Vector2f(50 + i * 60, 50), 1));
    }
    // Add level-specific blocks
    if (level == 1) {
        for (int i = 0; i < 5; ++i) {
            blocks.push_back(new Block(sf::Vector2f(50, 20), sf::Vector2f(100 + i * 60, 100), 10));
        }
    } else if (level == 2) {
        for (int i = 0; i < 5; ++i) {
            blocks.push_back(new Block(sf::Vector2f(50, 20), sf::Vector2f(100 + i * 60, 100), 10));
            blocks.push_back(new Block(sf::Vector2f(50, 20), sf::Vector2f(100 + i * 60, 130), 10));
        }
    } else if (level == 3) {
        for (int i = 0; i < 5; ++i) {
            blocks.push_back(new Block(sf::Vector2f(50, 20), sf::Vector2f(100 + i * 60, 100), 10));
            blocks.push_back(new Block(sf::Vector2f(50, 20), sf::Vector2f(100 + i * 60, 130), 10));
            blocks.push_back(new SpecialBlock(sf::Vector2f(50, 20), sf::Vector2f(100 + i * 60, 160), 10)); // Special block
        }
    } else if (level == 4) {
        for (int i = 0; i < 5; ++i) {
            blocks.push_back(new Block(sf::Vector2f(50, 20), sf::Vector2f(100 + i * 60, 100), 10));
            blocks.push_back(new Block(sf::Vector2f(50, 20), sf::Vector2f(100 + i * 60, 130), 10));
            blocks.push_back(new SpecialBlock(sf::Vector2f(50, 20), sf::Vector2f(100 + i * 60, 160), 10));
            blocks.push_back(new BoomBlock(sf::Vector2f(50, 20), sf::Vector2f(100 + i * 60, 190), 1));
        }
    } else if (level == 5) { // Boss level
        for (int i = 0; i < 10; ++i) {
            blocks.push_back(new SpecialBlock(sf::Vector2f(50, 20), sf::Vector2f(50 + i * 60, 100), 10));
        }
        for (int i = 0; i < 10; ++i) {
            blocks.push_back(new BoomBlock(sf::Vector2f(50, 20), sf::Vector2f(50 + i * 60, 130), 1));
        }
        for (int i = 0; i < 5; ++i) {
            blocks.push_back(new MovingBlock(sf::Vector2f(50, 20), sf::Vector2f(100 + i * 60, 160), 20, sf::Vector2f(2.f, 0.f))); // Moving block
        }
    }
    // Add more levels as needed
}

void loadBackground(sf::Texture &backgroundTexture, sf::Sprite &backgroundSprite, int level) {
    std::string filename = "background" + std::to_string(level) + ".png";
    if (!backgroundTexture.loadFromFile(filename)) {
        // Fallback to a default background if the specific one is not found
        backgroundTexture.loadFromFile("background.png");
    }
    backgroundSprite.setTexture(backgroundTexture);
}

void showSettings(sf::RenderWindow &window, sf::Font &font, std::vector<sf::CircleShape> &balls) {
    // Display settings menu
    sf::Text settingsText;
    settingsText.setFont(font);
    settingsText.setCharacterSize(24);
    settingsText.setFillColor(sf::Color::White);
    settingsText.setPosition(10, 10);
    settingsText.setString("Settings:\n1. Sound On/Off\n2. Volume Up\n3. Volume Down\n4. Language: English/Bangla\n5. Ball Color: Red\n6. Ball Color: Green\n7. Ball Color: Blue\n8. Back to Game");

    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed || sf::Keyboard::isKeyPressed(sf::Keyboard::Escape))
                window.close();
            if (event.type == sf::Event::KeyPressed) {
                if (event.key.code == sf::Keyboard::Num1) {
                    soundOn = !soundOn;
                }
                if (event.key.code == sf::Keyboard::Num2) {
                    volume = std::min(volume + 10.f, 100.f);
                }
                if (event.key.code == sf::Keyboard::Num3) {
                    volume = std::max(volume - 10.f, 0.f);
                }
                if (event.key.code == sf::Keyboard::Num4) {
                    language = (language == "English") ? "Bangla" : "English";
                }
                if (event.key.code == sf::Keyboard::Num5) {
                    ballColor = sf::Color::Red;
                    for (auto &ball : balls) {
                        ball.setFillColor(ballColor);
                    }
                }
                if (event.key.code == sf::Keyboard::Num6) {
                    ballColor = sf::Color::Green;
                    for (auto &ball : balls) {
                        ball.setFillColor(ballColor);
                    }
                }
                if (event.key.code == sf::Keyboard::Num7) {
                    ballColor = sf::Color::Blue;
                    for (auto &ball : balls) {
                        ball.setFillColor(ballColor);
                    }
                }
                if (event.key.code == sf::Keyboard::Num8) {
                    return; // Back to game
                }
            }
        }

        window.clear();
        window.draw(settingsText);
        window.display();
    }
}

int main(int argc, char const *argv[]) {
    sf::RenderWindow window(sf::VideoMode(800, 600), "SFML works!");
    std::vector<sf::CircleShape> balls;
    sf::CircleShape shape(10.f);
    shape.setPosition(window.getSize().x / 2 - 10, window.getSize().y - 10 * 2 - 5);
    shape.setFillColor(ballColor);
    balls.push_back(shape);

    sf::RectangleShape rect(sf::Vector2f(50, 5));
    rect.setPosition(window.getSize().x / 2 - 25, window.getSize().y - 5);
    rect.setFillColor(sf::Color::Green);
    bool startMove = false;

    // Create blocks
    std::vector<Block*> blocks;
    int level = 1;
    initializeBlocks(blocks, level);

    // Clock to track elapsed time
    sf::Clock clock;
    float speedIncreaseInterval = 10.f; // Increase speed every 10 seconds
    float speedMultiplier = 1.1f; // Increase speed by 10%

    int lives = 3; // Number of lives
    int score = 0; // Player's score

    // Load font for displaying score
    sf::Font font;
    if (!font.loadFromFile("arial.ttf")) {
        return -1; // Error loading font
    }

    // Create text to display score
    sf::Text scoreText;
    scoreText.setFont(font);
    scoreText.setCharacterSize(24);
    scoreText.setFillColor(sf::Color::White);
    scoreText.setPosition(10, 10);

    // Load background texture
    sf::Texture backgroundTexture;
    sf::Sprite backgroundSprite;
    loadBackground(backgroundTexture, backgroundSprite, level);

    // Load sound buffers
    sf::SoundBuffer hitBuffer, lifeLossBuffer, bottomHitBuffer;
    if (!hitBuffer.loadFromFile("hit.wav") || !lifeLossBuffer.loadFromFile("life_loss.wav") || !bottomHitBuffer.loadFromFile("bottom_hit.wav")) {
        return -1; // Error loading sound buffers
    }

    // Create sounds
    sf::Sound hitSound(hitBuffer);
    sf::Sound lifeLossSound(lifeLossBuffer);
    sf::Sound bottomHitSound(bottomHitBuffer);
    hitSound.setVolume(volume);
    lifeLossSound.setVolume(volume);
    bottomHitSound.setVolume(volume);

    // Load highest score
    highestScore = loadHighScore();

    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed || sf::Keyboard::isKeyPressed(sf::Keyboard::Escape))
                window.close();
            if (event.type == sf::Event::KeyPressed) {
                if (event.key.code == sf::Keyboard::Left) {
                    rect.move(-10, 0);
                    if (!startMove) {
                        for (auto &ball : balls) {
                            ball.move(-10, 0);
                        }
                    }
                }
                if (event.key.code == sf::Keyboard::Right) {
                    rect.move(10, 0);
                    if (!startMove) {
                        for (auto &ball : balls) {
                            ball.move(10, 0);
                        }
                    }
                }
                if (event.key.code == sf::Keyboard::Space) {
                    startMove = true;
                    clock.restart(); // Restart the clock when the game starts
                }
                if (event.key.code == sf::Keyboard::S) {
                    showSettings(window, font, balls); // Show settings menu
                }
            }
        }

        // Increase ball speed after a certain interval
        if (clock.getElapsedTime().asSeconds() > speedIncreaseInterval) {
            ballVelocity *= speedMultiplier;
            clock.restart(); // Restart the clock after increasing speed
        }

        if (startMove)
            autoMoveBall(balls, window, blocks, rect, lives, score, hitSound, lifeLossSound, bottomHitSound);

        // Check if the player has run out of lives
        if (lives <= 0) {
            if (score > highestScore) {
                highestScore = score;
                saveHighScore(highestScore);
            }
            window.close();
        }

        // Check if all blocks are destroyed to move to the next level
        if (blocks.empty()) {
            level++;
            initializeBlocks(blocks, level);
            loadBackground(backgroundTexture, backgroundSprite, level); // Load new background for the level
            startMove = false; // Stop the ball movement until the player presses space again
            shape.setPosition(window.getSize().x / 2 - 10, window.getSize().y - 10 * 2 - 5); // Reset ball position
            ballVelocity = sf::Vector2f(0.f, -10.f); // Reset ball velocity
        }

        // Update the score text
        scoreText.setString("Score: " + std::to_string(score) + " Lives: " + std::to_string(lives) + "\nPlayer: " + playerName + "\nHighest Score: " + std::to_string(highestScore));

        window.clear();
        window.draw(backgroundSprite); 
        for (const auto &ball : balls) {
            window.draw(ball);
        }
        window.draw(rect);
        for (const auto &block : blocks) {
            window.draw(block->shape);
        }
        window.draw(scoreText);
        window.display();
    }

    for (auto block : blocks) {
        delete block;
    }

    return 0;
}
