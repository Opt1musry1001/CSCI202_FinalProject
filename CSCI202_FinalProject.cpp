/*
Author: Ryan Moore
Date: 12/14/23
File: CSCI202_FinalProject.cpp
Related Files: high_scores.txt, questions.txt
Purpose: Trivia game that allows the user to play a quiz, add questions, or view high scores.
*/


#include <iostream>
#include <fstream>
#include <vector>
#include <sstream>
#include <random>
#include <chrono>
#include <algorithm>


// Shuffle the elements of a container
template <typename RandomIt>
void shuffleQuestions(RandomIt first, RandomIt last) {
    // Use a random seed based on the current time
    auto seed = std::chrono::system_clock::now().time_since_epoch().count();
    std::default_random_engine rng(seed);

    // Shuffle the elements
    std::shuffle(first, last, rng);
}

// Base question class
class Question {
public:
    std::string question;
    std::string answer;

    Question(const std::string& q, const std::string& a) : question(q), answer(a) {}

    virtual void askQuestion() const {
        std::cout << "Question: " << question << std::endl;
    }

    virtual bool checkAnswer(const std::string& userAnswer) const {
        // Convert both answers to uppercase
        std::string userAnswerUpper = userAnswer;
        std::transform(userAnswerUpper.begin(), userAnswerUpper.end(), userAnswerUpper.begin(), ::toupper);

        std::string correctAnswerUpper = answer;
        std::transform(correctAnswerUpper.begin(), correctAnswerUpper.end(), correctAnswerUpper.begin(), ::toupper);

        return userAnswerUpper == correctAnswerUpper;
    }

    virtual std::string getAnswer() const {
        return answer;
    }
};

// Category-specific question class
class CategoryQuestion : public Question {
public:
    std::string category;

    CategoryQuestion(const std::string& q, const std::string& a, const std::string& cat)
        : Question(q, a), category(cat) {}

    void askQuestion() const override {
        std::cout << "Category: " << category << std::endl;
        Question::askQuestion();
    }

    virtual std::string getAnswer() const override {
        return answer;
    }
};

class QuizGame {
private:
    std::vector<Question*> questions;

    // Load questions from a file
    void loadQuestionsFromFile(const std::string& filename) {
        std::ifstream file(filename);

        if (!file.is_open()) {
            std::cerr << "Error opening file: " << filename << std::endl;
            return;
        }

        std::string line;
        while (std::getline(file, line)) {
            // Parse the line and create question objects
            std::istringstream iss(line);
            std::string question, answer, category;

            // Split the line using the delimiter '|'
            if (std::getline(iss, question, '|') &&
                std::getline(iss, answer, '|') &&
                std::getline(iss, category)) {

                // Create a CategoryQuestion object and add it to the vector
                questions.push_back(new CategoryQuestion(question, answer, category));
            } else {
                // Handle invalid lines
                std::cerr << "Invalid line in file: " << line << std::endl;
            }
        }

        file.close();
    }

    // Save high scores to a file
    void saveHighScores(const std::string& filename, int score) {
        std::ifstream inFile(filename);
        std::ofstream outFile("temp.txt");

        // Read existing high scores
        std::vector<std::pair<int, std::string>> highScores; // Pair of score and player name

        int highScore;
        std::string playerName;

        while (inFile >> highScore) {
            getline(inFile >> std::ws, playerName);  // Read the rest of the line (including potential spaces)
            highScores.emplace_back(highScore, playerName);
        }

        inFile.close();

        // Add the new score
        highScores.emplace_back(score, "Player");

        // Sort the high scores first by score (descending) and then by the order of insertion
        std::sort(highScores.begin(), highScores.end(),
                [](const auto& lhs, const auto& rhs) {
                    return (lhs.first > rhs.first) || ((lhs.first == rhs.first) && (lhs.second < rhs.second));
                });

        // Determine the position of the new score
        auto newScorePosition = std::find_if(highScores.begin(), highScores.end(),
                                            [score](const auto& entry) { return entry.first == score; });

        // If the new score is not already in the list, insert it at the correct position
        if (newScorePosition == highScores.end()) {
            newScorePosition = std::find_if(highScores.begin(), highScores.end(),
                                            [score](const auto& entry) { return entry.first < score; });

            highScores.insert(newScorePosition, std::make_pair(score, "Player"));
        }

        // Check if the new score is within the top 10 before asking for the new high scorer's name
        if (newScorePosition != highScores.end() && std::distance(highScores.begin(), newScorePosition) < 10) {
            std::cout << "Congratulations! You made it to the leaderboard.\n";
            std::cout << "Enter your name: ";
            std::getline(std::cin, newScorePosition->second);
        }

        // Save only the top 10 scores
        for (size_t i = 0; i < std::min(highScores.size(), static_cast<size_t>(10)); ++i) {
            outFile << highScores[i].first << " " << highScores[i].second << std::endl;
        }

        outFile.close();

        // Replace the original file with the updated scores
        std::remove(filename.c_str());
        std::rename("temp.txt", filename.c_str());
    }

public:
    QuizGame(const std::string& questionFile) {
        // Load questions from the file
        loadQuestionsFromFile(questionFile);
    }

    ~QuizGame() {
        // Clean up dynamically allocated question objects
        for (Question* q : questions) {
            delete q;
        }
    }

    // Function to play the game
    void playGame() {
        std::cout << "Welcome to the Quiz Game!\n";

        int score = 0;
        const int maxQuestions = 20;  // Adjust the number of questions as needed
        const int timeLimitSeconds = 15;  // Adjust the time limit as needed

        // Shuffle the questions vector to randomize the order
        shuffleQuestions(questions.begin(), questions.end());

        for (int i = 0; i < std::min(maxQuestions, static_cast<int>(questions.size())); ++i) {
            // Display current score and ask the question
            std::cout << "\nCurrent Score: " << score << "\n\n";
            questions[i]->askQuestion();

            // Start the timer
            auto start = std::chrono::high_resolution_clock::now();

            // Get user's answer
            std::string userAnswer;
            std::cout << "Your answer: ";
            std::cin >> std::ws;  // Clear leading whitespaces, including newline characters
            std::getline(std::cin, userAnswer);

            // Stop the timer
            auto end = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double> elapsedSeconds = end - start;

            // Check the answer and update the score based on time
            if (questions[i]->checkAnswer(userAnswer)) {
                std::cout << "Correct!\n";

                // Calculate points based on time
                int timeBonus = static_cast<int>(std::max(0.0, timeLimitSeconds - elapsedSeconds.count()));
                score += 10 + timeBonus;  // Adjust the points as needed
            } else {
                std::cout << "Incorrect!\n";
                std::cout << "The correct answer is: " << questions[i]->getAnswer() << "\n";
            }

            // Check if the user ran out of time
            if (elapsedSeconds.count() >= timeLimitSeconds) {
                std::cout << "Out of time! No points awarded for this question.\n";
            }
        }

        // Display final score and save high scores
        std::cout << "\nGame Over! Final Score: " << score << "\n\n";
        saveHighScores("high_scores.txt", score);
    }

    // Function to add a new question
    void addQuestion() {
        std::string question, answer, category;

        // Prompt the user for input
        std::cout << "\nEnter the new question: ";
        std::cin.ignore();  // Clear any remaining newline characters in the buffer
        std::getline(std::cin, question);

        std::cout << "Enter the answer: ";
        std::getline(std::cin, answer);

        // Prompt the user to select the category
        std::cout << "Select the category:\n"
                  << "1. History\n"
                  << "2. Geography\n"
                  << "3. Science\n"
                  << "4. Entertainment\n"
                  << "5. Art\n";

        int categoryChoice;
        std::cout << "Enter the number corresponding to the category: ";
        std::cin >> categoryChoice;

        // Convert the user's choice to the actual category name
        switch (categoryChoice) {
            case 1:
                category = "History";
                break;
            case 2:
                category = "Geography";
                break;
            case 3:
                category = "Science";
                break;
            case 4:
                category = "Entertainment";
                break;
            case 5:
                category = "Art";
                break;
            default:
                std::cerr << "Invalid category choice.\n";
                return;
        }

        // Create a new CategoryQuestion object and add it to the vector
        questions.push_back(new CategoryQuestion(question, answer, category));

        // Append the new question to the questions.txt file
        std::ofstream questionFile("questions.txt", std::ios::app);
        if (questionFile.is_open()) {
            // Format: question|answer|category
            questionFile << question << "|" << answer << "|" << category << "\n";
            questionFile.close();
            std::cout << "Question added successfully!\n";
        } else {
            std::cerr << "Unable to open questions.txt for appending.\n";
        }
    }

    // Function to view the high scores
    void displayHighScores(const std::string& filename) const {
        std::ifstream inFile(filename);
        if (inFile.is_open()) {
            std::cout << "\nHigh Scores:\n";
            int rank = 1;
            int score;
            std::string playerName;

            while (inFile >> score) {
                getline(inFile >> std::ws, playerName);  // Read the rest of the line (including potential spaces)
                std::cout << rank << ". " << playerName << ": " << score << " points\n";
                rank++;
            }

            inFile.close();
        } else {
            std::cerr << "\nUnable to open high scores file.\n";
        }
    }

    void displayGameRules() const {
        std::cout << "\nWelcome to the Quiz Game!\n\n"
                << "Game Rules:\n"
                << "1. You will be asked a series of 20 random questions.\n"
                << "2. Each correct answer earns you points.\n"
                << "3. The faster you answer, the more points you get.\n"
                << "4. There is a 15 second time limit for each question.\n"
                << "5. If you don't answer in time, no points are awarded.\n"
                << "6. Capitalization doesn't matter for answers.\n"
                << "7. After the game, your score may be added to the leaderboard.\n"
                << "8. To add new questions to the game, choose option 2 and follow the instructions.\n"
                << "9. Ensure correct spelling for new questions.\n\n";
    }
};

int main() {
    // File containing questions
    const std::string questionFile = "questions.txt";

    // Create a quiz game
    QuizGame game(questionFile);

    int choice;
    do {
        // Display menu
        std::cout << "\nWhat Would You Like to Do?\n"
                  << "1. Play the Game\n"
                  << "2. Add Questions\n"
                  << "3. View High Scores\n"
                  << "4. Game Rules\n"
                  << "5. Quit\n"
                  << "Enter your choice: ";

        // Input validation
        while (!(std::cin >> choice)) {
            std::cin.clear();  // Clear the error flag
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');  // Discard invalid input
            std::cout << "Invalid input. Please enter a number: ";
        }

        switch (choice) {
        case 1:
            // Play the game
            game.playGame();
            break;
        case 2:
            // Add questions
            game.addQuestion();
            break;
        case 3:
            // Display high scores
            game.displayHighScores("high_scores.txt");
            break;
        case 4:
            // Display game rules
            game.displayGameRules();
            break;
        case 5:
            // Quit
            std::cout << "Goodbye!\n";
            break;
        default:
            std::cout << "Invalid choice. Try again.\n";
    }
    } while (choice != 5);

    return 0;
}
