#include <stack>
#include <queue>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <SFML/Graphics.hpp>
#include <iostream>
#include <string>

#include "User.h"
#include "Post.h"
#include "Theme.h"

using namespace std;

enum Page
{
    LOGIN,
    REGISTER,      // registration page (Increment 1)
    SPLASH,
    HOME,
    STORY_VIEW,
    POST_VIEW,
    HELP,
    ABOUT,
    NOTIFICATIONS, // notifications page (Increment 2)
    EDIT_PROFILE,  // NEW: Name / Bio / Profile Picture / Password
    CREATE_POST    // NEW: Caption + Select Image + Post (replaces the "press A" flow)
};

queue<string> stories;
stack<Page> pageHistory;

// ==================================================================
// Notifications
// ------------------------------------------------------------------
// DSA: Queue
// WHY: Notifications arrive in the order events happen and should be
//      read in that same order (oldest activity first, like a real
//      notification feed scrolling top to bottom as new ones push
//      in) -- a queue's FIFO behavior maps directly onto that, and
//      we never need to jump to an arbitrary notification, only walk
//      front-to-back or clear everything.
// Time complexity: push() O(1), full read (front-to-back) O(n).
// ==================================================================
queue<string> notifications;

// Strips leading/trailing spaces so an accidental space typed (or pasted)
// before/after a username or password doesn't cause a false "wrong
// password" result.
static string trimSpaces(const string& s)
{
    size_t start = s.find_first_not_of(' ');
    if (start == string::npos) return "";
    size_t end = s.find_last_not_of(' ');
    return s.substr(start, end - start + 1);
}

int main()
{
    sf::RenderWindow window(
        sf::VideoMode({1600, 900}),
        "Social Media Feed - Linked List Project"
    );

    window.setFramerateLimit(60);

    sf::Font font;

    if (!font.openFromFile("arial.ttf"))
    {
        cout << "Font not loaded!\n";
        return -1;
    }

    sf::Texture profileTexture;
    if (!profileTexture.loadFromFile("profile.jpg"))
        cout << "Profile image not loaded!\n";

    sf::Texture pizzaTexture;
    sf::Texture coffeeTexture;
    sf::Texture catTexture;

    if (!pizzaTexture.loadFromFile("pizza.jpg"))
        cout << "Pizza image not loaded!\n";

    if (!coffeeTexture.loadFromFile("coffee.jpg"))
        cout << "Coffee image not loaded!\n";

    if (!catTexture.loadFromFile("cat.jpg"))
        cout << "Cat image not loaded!\n";

    Feed feed;

    stories.push("Maryam");
    stories.push("Pizza Lover");
    stories.push("Code Ninja");
    stories.push("Coffee Addict");
    stories.push("Cat Fan");

    queue<string> friendSuggestions;
    friendSuggestions.push("Ali");
    friendSuggestions.push("Ahmad");
    friendSuggestions.push("Saman");
    friendSuggestions.push("Hifza");
    friendSuggestions.push("Momina");

    sf::Texture instaLogo;
    if (!instaLogo.loadFromFile("instagram_logo.png"))
        cout << "Instagram logo not loaded!\n";

    sf::Texture storyTexture;
    if (!storyTexture.loadFromFile("story.jpg"))
        cout << "Story image not loaded!\n";

    // Pictures the user can pick from for their profile picture or a new
    // post (SFML has no native OS file-picker, so selection cycles through
    // the images already bundled with the project).
    struct PicOption { string label; sf::Texture* tex; };
    vector<PicOption> pictureOptions = {
        { "Default",  &profileTexture },
        { "Pizza",    &pizzaTexture   },
        { "Coffee",   &coffeeTexture  },
        { "Cat",      &catTexture     },
        { "Story",    &storyTexture   }
    };

    feed.addPost("Pizza Lover", "Life is better with pizza", &pizzaTexture);
    feed.addPost("Code Ninja", "Fixed bugs", &profileTexture);
    feed.addPost("Coffee Addict", "Coffee life", &coffeeTexture);
    feed.addPost("Cat Fan", "Meow world", &catTexture);

    // ================ USER MANAGER (NEW) ================
    // Real accounts, stored in a Linked List (see User.h/User.cpp).
    UserManager userManager;
    userManager.seedDemoUsers();   // Maryam/1234, Ali/1111, Sara/2222, ...
    userManager.loadUsers();       // <-- ADD THIS LINE
    User* currentUser = nullptr;

    Page currentPage = LOGIN;
    sf::Clock splashClock;
    float scrollOffset = 0.f;
    float maxScroll = 0.f;
    string currentStory = "";
    Post* selectedPost = nullptr;
    Post* searchedPost = nullptr;
    string searchInput = "";
    bool typingSearch = false;
    sf::Texture* currentStoryTexture = nullptr;

    // ================ LOGIN PAGE VARIABLES ================
    string loginUsername = "";
    string loginPassword = "";
    int activeField = 0;       // 0 = username field, 1 = password field
    string loginErrorMsg = "";
    bool showPassword = false;

    // ================ REGISTER PAGE VARIABLES (NEW) ================
    string regUsername = "";
    string regPassword = "";
    string regConfirm = "";
    int regActiveField = 0;    // 0 = username, 1 = password, 2 = confirm
    string regErrorMsg = "";
    string regSuccessMsg = "";

    // ================ EDIT PROFILE PAGE VARIABLES (NEW) ================
    string editName = "";
    string editBio = "";
    int editPicIndex = 0;
    string editOldPass = "";
    string editNewPass = "";
    string editConfirmPass = "";
    int editActiveField = 0;   // 0=Name 1=Bio 2=OldPass 3=NewPass 4=ConfirmPass
    string editErrorMsg = "";
    string editSuccessMsg = "";

    // ================ CREATE POST PAGE VARIABLES (NEW) ================
    string newPostCaption = "";
    int newPostPicIndex = 0;
    bool typingCaption = false;
    string createPostMsg = "";

    // Loads the currently logged-in user's saved values into the Edit
    // Profile fields -- called every time we enter that page so it never
    // shows stale data from a previous visit or a different user.
    auto openEditProfile = [&]()
    {
        if (currentUser)
        {
            editName = currentUser->displayName;
            editBio = currentUser->bio;

            editPicIndex = 0;
            for (size_t p = 0; p < pictureOptions.size(); p++)
                if (pictureOptions[p].label == currentUser->profilePicPath)
                    editPicIndex = static_cast<int>(p);
        }

        editOldPass = "";
        editNewPass = "";
        editConfirmPass = "";
        editActiveField = 0;
        editErrorMsg = "";
        editSuccessMsg = "";
        currentPage = EDIT_PROFILE;
    };

    while (window.isOpen())
    {
        if (currentPage == SPLASH)
        {
            if (splashClock.getElapsedTime().asSeconds() > 3)
                currentPage = HOME;
        }

        while (auto event = window.pollEvent())
        {
            if (event->is<sf::Event::KeyPressed>())
            {
                auto key = event->getIf<sf::Event::KeyPressed>();

                if (key->code == sf::Keyboard::Key::T && currentPage == HOME && !typingSearch)
                {
                    currentPage = SPLASH;
                    splashClock.restart();
                }

                if (key->code == sf::Keyboard::Key::Tab && currentPage == LOGIN)
                    activeField = (activeField == 0) ? 1 : 0;

                if (key->code == sf::Keyboard::Key::Tab && currentPage == REGISTER)
                    regActiveField = (regActiveField + 1) % 3;

                if (key->code == sf::Keyboard::Key::Tab && currentPage == EDIT_PROFILE)
                    editActiveField = (editActiveField + 1) % 5;
            }

            // ================ LOGIN PAGE TEXT INPUT ================
            if (event->is<sf::Event::TextEntered>() && currentPage == LOGIN)
            {
                auto textEvent = event->getIf<sf::Event::TextEntered>();
                unsigned int unicode = textEvent->unicode;

                string* target = (activeField == 0) ? &loginUsername : &loginPassword;

                if (unicode == 8)
                {
                    if (!target->empty())
                        target->pop_back();
                }
                else if (unicode == 9)
                {
                    // handled in KeyPressed
                }
                else if (unicode == 13) // Enter -> real authentication
                {
                    string uname = trimSpaces(loginUsername);
                    string pass = trimSpaces(loginPassword);
                    User* existing = userManager.findUser(uname);

                    if (!existing)
                    {
                        loginErrorMsg = "No account found for this username. Please create one below.";
                    }
                    else if (existing->password != pass)
                    {
                        loginErrorMsg = "Incorrect password. Please try again.";
                    }
                    else
                    {
                        currentUser = existing;
                        loginErrorMsg = "";
                        currentPage = SPLASH;
                        splashClock.restart();
                    }
                }
                else if (unicode >= 32 && unicode < 128)
                {
                    if (target->size() < 22)
                        target->push_back(static_cast<char>(unicode));
                }
            }

            // ================ REGISTER PAGE TEXT INPUT (NEW) ================
            if (event->is<sf::Event::TextEntered>() && currentPage == REGISTER)
            {
                auto textEvent = event->getIf<sf::Event::TextEntered>();
                unsigned int unicode = textEvent->unicode;

                string* target =
                    (regActiveField == 0) ? &regUsername :
                    (regActiveField == 1) ? &regPassword  : &regConfirm;

                if (unicode == 8)
                {
                    if (!target->empty())
                        target->pop_back();
                }
                else if (unicode == 13) // Enter -> attempt registration
                {
                    if (regUsername.empty() || regPassword.empty() || regConfirm.empty())
                    {
                        regErrorMsg = "All fields are required!";
                        regSuccessMsg = "";
                    }
                    else if (regPassword != regConfirm)
                    {
                        regErrorMsg = "Passwords do not match!";
                        regSuccessMsg = "";
                    }
                    else if (userManager.findUser(regUsername) != nullptr)
                    {
                        regErrorMsg = "Username already taken!";
                        regSuccessMsg = "";
                    }
                    else
                    {
                        
                        userManager.registerUser(regUsername, regPassword);
                        userManager.saveUsers();
                        regErrorMsg = "";
                        regSuccessMsg = "Account created! You can log in now.";
                        regUsername = "";
                        regPassword = "";
                        regConfirm = "";
                    }
                }
                else if (unicode >= 32 && unicode < 128)
                {
                    if (target->size() < 22)
                        target->push_back(static_cast<char>(unicode));
                }
            }

            // ================ EDIT PROFILE TEXT INPUT (NEW) ================
            if (event->is<sf::Event::TextEntered>() && currentPage == EDIT_PROFILE)
            {
                auto textEvent = event->getIf<sf::Event::TextEntered>();
                unsigned int unicode = textEvent->unicode;

                string* target =
                    (editActiveField == 0) ? &editName :
                    (editActiveField == 1) ? &editBio :
                    (editActiveField == 2) ? &editOldPass :
                    (editActiveField == 3) ? &editNewPass : &editConfirmPass;

                size_t maxLen = (editActiveField == 1) ? 80 : 22;

                if (unicode == 8)
                {
                    if (!target->empty())
                        target->pop_back();
                }
                else if (unicode >= 32 && unicode < 128)
                {
                    if (target->size() < maxLen)
                        target->push_back(static_cast<char>(unicode));
                }
            }

            // ================ CREATE POST CAPTION TEXT INPUT (NEW) ================
            if (event->is<sf::Event::TextEntered>() && currentPage == CREATE_POST && typingCaption)
            {
                auto textEvent = event->getIf<sf::Event::TextEntered>();
                unsigned int unicode = textEvent->unicode;

                if (unicode == 8)
                {
                    if (!newPostCaption.empty())
                        newPostCaption.pop_back();
                }
                else if (unicode >= 32 && unicode < 128)
                {
                    if (newPostCaption.size() < 120)
                        newPostCaption.push_back(static_cast<char>(unicode));
                }
            }

            // ================ SEARCH BOX TEXT INPUT (any user) ================
            if (event->is<sf::Event::TextEntered>() && currentPage != LOGIN &&
                currentPage != REGISTER && typingSearch)
            {
                auto textEvent = event->getIf<sf::Event::TextEntered>();
                unsigned int unicode = textEvent->unicode;

                if (unicode == 8)
                {
                    if (!searchInput.empty())
                        searchInput.pop_back();
                }
                else if (unicode == 13)
                {
                    searchedPost = feed.searchPost(searchInput);

                    if (searchedPost)
                        cout << "Post Found!" << endl;
                    else
                        cout << "Not Found!" << endl;
                }
                else if (unicode >= 32 && unicode < 128)
                {
                    if (searchInput.size() < 24)
                        searchInput.push_back(static_cast<char>(unicode));
                }
            }

            if (event->is<sf::Event::MouseWheelScrolled>())
            {
                auto wheel = event->getIf<sf::Event::MouseWheelScrolled>();

                scrollOffset -= wheel->delta * 40.f;

                if (scrollOffset < 0)
                    scrollOffset = 0;
                if (scrollOffset > maxScroll)
                    scrollOffset = maxScroll;
            }

            if (event->is<sf::Event::Closed>())
                window.close();

            if (event->is<sf::Event::KeyPressed>())
            {
                auto key = event->getIf<sf::Event::KeyPressed>();
                bool onFeedPage = currentPage == HOME && !typingSearch;

                if (key->code == sf::Keyboard::Key::D && onFeedPage)
                    feed.deleteLastPost();

                if (key->code == sf::Keyboard::Key::S && onFeedPage)
                {
                    searchedPost = feed.searchPost(searchInput);

                    if (searchedPost)
                        cout << "Post Found!" << endl;
                    else
                        cout << "Not Found!" << endl;
                }

                if (key->code == sf::Keyboard::Key::R && onFeedPage)
                {
                    searchedPost = nullptr;
                    searchInput = "";
                }

                if (key->code == sf::Keyboard::Key::B && onFeedPage)
                {
                    feed.sortByLikes();
                }

                if (key->code == sf::Keyboard::Key::Escape && onFeedPage)
                    currentPage = HOME;
            }

            if (event->is<sf::Event::MouseButtonPressed>())
            {
                auto mouse = sf::Mouse::getPosition(window);

                // ================ LOGIN PAGE CLICKS ================
                if (currentPage == LOGIN)
                {
                    if (mouse.x >= 600 && mouse.x <= 1000 &&
                        mouse.y >= 370 && mouse.y <= 415)
                    {
                        activeField = 0;
                    }
                    else if (mouse.x >= 600 && mouse.x <= 1000 &&
                             mouse.y >= 440 && mouse.y <= 485)
                    {
                        activeField = 1;
                    }
                    else if (mouse.x >= 1010 && mouse.x <= 1100 &&
                             mouse.y >= 440 && mouse.y <= 485)
                    {
                        showPassword = !showPassword;
                    }
                    else if (mouse.x >= 700 && mouse.x <= 900 &&
                             mouse.y >= 530 && mouse.y <= 575)
                    {
                        string uname = trimSpaces(loginUsername);
                        string pass = trimSpaces(loginPassword);
                        User* existing = userManager.findUser(uname);

                        if (!existing)
                        {
                            loginErrorMsg = "No account found for this username. Please create one below.";
                        }
                        else if (existing->password != pass)
                        {
                            loginErrorMsg = "Incorrect password. Please try again.";
                        }
                        else
                        {
                            currentUser = existing;
                            loginErrorMsg = "";
                            currentPage = SPLASH;
                            splashClock.restart();
                        }
                    }
                    // "Create Account" link -> go to Register page
                    else if (mouse.x >= 700 && mouse.x <= 900 &&
                             mouse.y >= 610 && mouse.y <= 635)
                    {
                        regErrorMsg = "";
                        regSuccessMsg = "";
                        currentPage = REGISTER;
                    }
                }
                // ================ REGISTER PAGE CLICKS (NEW) ================
                else if (currentPage == REGISTER)
                {
                    if (mouse.x >= 600 && mouse.x <= 1000 &&
                        mouse.y >= 320 && mouse.y <= 365)
                    {
                        regActiveField = 0;
                    }
                    else if (mouse.x >= 600 && mouse.x <= 1000 &&
                             mouse.y >= 390 && mouse.y <= 435)
                    {
                        regActiveField = 1;
                    }
                    else if (mouse.x >= 600 && mouse.x <= 1000 &&
                             mouse.y >= 460 && mouse.y <= 505)
                    {
                        regActiveField = 2;
                    }
                    // REGISTER button
                    else if (mouse.x >= 700 && mouse.x <= 900 &&
                             mouse.y >= 540 && mouse.y <= 585)
                    {
                        if (regUsername.empty() || regPassword.empty() || regConfirm.empty())
                        {
                            regErrorMsg = "All fields are required!";
                            regSuccessMsg = "";
                        }
                        else if (regPassword != regConfirm)
                        {
                            regErrorMsg = "Passwords do not match!";
                            regSuccessMsg = "";
                        }
                        else if (userManager.findUser(regUsername) != nullptr)
                        {
                            regErrorMsg = "Username already taken!";
                            regSuccessMsg = "";
                        }
                     else
                        {
                            userManager.registerUser(regUsername, regPassword);
                            userManager.saveUsers();
                            regErrorMsg = "";
                            regSuccessMsg = "Account created! You can log in now.";
                            regUsername = "";
                            regPassword = "";
                            regConfirm = "";
                        }
                    }
                    // "Back to Login" link
                    else if (mouse.x >= 700 && mouse.x <= 900 &&
                             mouse.y >= 610 && mouse.y <= 635)
                    {
                        currentPage = LOGIN;
                    }
                }
                // POST VIEW: BACK button
                else if (currentPage == POST_VIEW)
                {
                    if (mouse.x >= 30 && mouse.x <= 150 &&
                        mouse.y >= 30 && mouse.y <= 70)
                    {
                        selectedPost = nullptr;
                        currentPage = HOME;
                    }
                }
                // STORY VIEW: BACK button
                else if (currentPage == STORY_VIEW)
                {
                    if (mouse.x >= 30 && mouse.x <= 150 &&
                        mouse.y >= 30 && mouse.y <= 70)
                    {
                        currentStory = "";
                        currentStoryTexture = nullptr;
                        currentPage = HOME;
                    }
                }
                // BACK button on HELP / ABOUT
                else if (currentPage == HELP || currentPage == ABOUT)
                {
                    if (mouse.x >= 30 && mouse.x <= 150 &&
                        mouse.y >= 30 && mouse.y <= 70)
                    {
                        currentPage = HOME;
                    }
                }
                // NOTIFICATIONS page: Back + Clear All
                else if (currentPage == NOTIFICATIONS)
                {
                    if (mouse.x >= 30 && mouse.x <= 150 &&
                        mouse.y >= 30 && mouse.y <= 70)
                    {
                        currentPage = HOME;
                    }
                    else if (mouse.x >= 1400 && mouse.x <= 1560 &&
                             mouse.y >= 30 && mouse.y <= 70)
                    {
                        while (!notifications.empty())
                            notifications.pop();
                    }
                }
                // ================ EDIT PROFILE PAGE CLICKS (NEW) ================
                // FIX: the click hit-boxes below now match the actual drawn
                // positions of each field (see drawField() calls in the
                // EDIT_PROFILE render block: Name=190, Bio=255,
                // Current Password=320, New Password=385,
                // Confirm New Password=450, each box is 42px tall).
                // Previously these hit-boxes were offset from the real
                // drawn boxes, so clicking the password fields (and SAVE)
                // did nothing.
                else if (currentPage == EDIT_PROFILE)
                {
                    if (mouse.x >= 30 && mouse.x <= 150 &&
                        mouse.y >= 30 && mouse.y <= 70)
                    {
                        currentPage = HOME;
                    }
                    // Name field (drawn at y=190, height 42)
                    else if (mouse.x >= 550 && mouse.x <= 1050 &&
                             mouse.y >= 190 && mouse.y <= 232)
                    {
                        editActiveField = 0;
                    }
                    // Bio field (drawn at y=255, height 42)
                    else if (mouse.x >= 550 && mouse.x <= 1050 &&
                             mouse.y >= 255 && mouse.y <= 297)
                    {
                        editActiveField = 1;
                    }
                    // Current Password field (drawn at y=320, height 42)
                    else if (mouse.x >= 550 && mouse.x <= 1050 &&
                             mouse.y >= 320 && mouse.y <= 362)
                    {
                        editActiveField = 2;
                    }
                    // New Password field (drawn at y=385, height 42)
                    else if (mouse.x >= 550 && mouse.x <= 1050 &&
                             mouse.y >= 385 && mouse.y <= 427)
                    {
                        editActiveField = 3;
                    }
                    // Confirm New Password field (drawn at y=450, height 42)
                    else if (mouse.x >= 550 && mouse.x <= 1050 &&
                             mouse.y >= 450 && mouse.y <= 492)
                    {
                        editActiveField = 4;
                    }
                    // SAVE button (drawn at y=610, height 45)
                    else if (mouse.x >= 700 && mouse.x <= 900 &&
                             mouse.y >= 610 && mouse.y <= 655 && currentUser)
                    {
                        string trimmedOld = trimSpaces(editOldPass);
                        string trimmedNew = trimSpaces(editNewPass);
                        string trimmedConfirm = trimSpaces(editConfirmPass);

                        bool wantsPasswordChange = !trimmedOld.empty() || !trimmedNew.empty() || !trimmedConfirm.empty();

                        if (editName.empty())
                        {
                            editErrorMsg = "Name cannot be empty.";
                            editSuccessMsg = "";
                        }
                        else if (wantsPasswordChange && trimmedOld != currentUser->password)
                        {
                            editErrorMsg = "Current password is incorrect.";
                            editSuccessMsg = "";
                        }
                        else if (wantsPasswordChange && trimmedNew.empty())
                        {
                            editErrorMsg = "New password cannot be empty.";
                            editSuccessMsg = "";
                        }
                        else if (wantsPasswordChange && trimmedNew != trimmedConfirm)
                        {
                            editErrorMsg = "New passwords do not match.";
                            editSuccessMsg = "";
                        }
                        else
                        {
                            currentUser->displayName = editName;
                            currentUser->bio = editBio;
                            currentUser->profilePicPath = pictureOptions[editPicIndex].label;

                            if (wantsPasswordChange)
                                currentUser->password = trimmedNew;

                            userManager.saveUsers();
                            editOldPass = "";
                            editNewPass = "";
                            editConfirmPass = "";
                            editErrorMsg = "";
                            editSuccessMsg = "Profile updated successfully!";
                        }
                    }
                }
                // ================ CREATE POST PAGE CLICKS (NEW) ================
                else if (currentPage == CREATE_POST)
                {
                    if (mouse.x >= 30 && mouse.x <= 150 &&
                        mouse.y >= 30 && mouse.y <= 70)
                    {
                        currentPage = HOME;
                    }
                    // HOME shortcut on this page too
                    else if (mouse.x >= 170 && mouse.x <= 290 &&
                             mouse.y >= 30 && mouse.y <= 70)
                    {
                        newPostCaption = "";
                        newPostPicIndex = 0;
                        typingCaption = false;
                        createPostMsg = "";
                        currentPage = HOME;
                    }
                    // Caption box
                    else if (mouse.x >= 550 && mouse.x <= 1300 &&
                             mouse.y >= 220 && mouse.y <= 320)
                    {
                        typingCaption = true;
                    }
                    // Select Image (cycles through available pictures)
                    else if (mouse.x >= 550 && mouse.x <= 760 &&
                             mouse.y >= 380 && mouse.y <= 425)
                    {
                        typingCaption = false;
                        newPostPicIndex = (newPostPicIndex + 1) % static_cast<int>(pictureOptions.size());
                    }
                    // POST button
                    else if (mouse.x >= 850 && mouse.x <= 1050 &&
                             mouse.y >= 430 && mouse.y <= 475)
                    {
                        typingCaption = false;

                        if (newPostCaption.empty())
                        {
                            createPostMsg = "Write a caption before posting.";
                        }
                        else
                        {
                            string authorName = currentUser ? currentUser->displayName : "Guest";
                            feed.addPost(authorName, newPostCaption, pictureOptions[newPostPicIndex].tex);
                            notifications.push(authorName + " created a new post");

                            newPostCaption = "";
                            newPostPicIndex = 0;
                            createPostMsg = "";
                            currentPage = HOME;
                        }
                    }
                    else
                    {
                        typingCaption = false;
                    }
                }
                else if (currentPage == HOME)
                {
                    float storyX = 80.f;
                    queue<string> tempStories = stories;
                    while (!tempStories.empty())
                    {
                        if (mouse.x >= storyX && mouse.x <= storyX + 70 &&
                            mouse.y >= 90 && mouse.y <= 160)
                        {
                            currentStory = tempStories.front();
                            currentPage = STORY_VIEW;

                            if (currentStory == "Maryam")
                                currentStoryTexture = &storyTexture;
                            else if (currentStory == "Pizza Lover")
                                currentStoryTexture = &pizzaTexture;
                            else if (currentStory == "Coffee Addict")
                                currentStoryTexture = &coffeeTexture;
                            else if (currentStory == "Cat Fan")
                                currentStoryTexture = &catTexture;
                            else if (currentStory == "Code Ninja")
                                currentStoryTexture = &profileTexture;

                            cout << currentStory << " Story Clicked\n";
                            break;
                        }

                        tempStories.pop();
                        storyX += 110.f;
                    }

                    if (mouse.x >= 1295 && mouse.x <= 1545 &&
                        mouse.y >= 460 && mouse.y <= 492)
                        typingSearch = true;
                    else
                        typingSearch = false;

                    // ================ CONTROL PANEL BUTTONS =================
                    if (mouse.x >= 1310 && mouse.x <= 1420 &&
                        mouse.y >= 680 && mouse.y <= 710)
                    {
                        newPostCaption = "";
                        newPostPicIndex = 0;
                        typingCaption = false;
                        createPostMsg = "";
                        currentPage = CREATE_POST;
                    }

                    if (mouse.x >= 1435 && mouse.x <= 1545 &&
                        mouse.y >= 680 && mouse.y <= 710)
                        feed.deleteLastPost();

                    if (mouse.x >= 1310 && mouse.x <= 1420 &&
                        mouse.y >= 720 && mouse.y <= 750)
                        searchedPost = feed.searchPost(searchInput);

                    if (mouse.x >= 1435 && mouse.x <= 1545 &&
                        mouse.y >= 720 && mouse.y <= 750)
                    {
                        searchedPost = nullptr;
                        searchInput = "";
                    }

                    if (mouse.x >= 1310 && mouse.x <= 1420 &&
                        mouse.y >= 760 && mouse.y <= 790)
                        feed.sortByLikes();

                    if (mouse.x >= 1435 && mouse.x <= 1545 &&
                        mouse.y >= 760 && mouse.y <= 790)
                    {
                        // HOME now does a real "go home" reset: back to the
                        // top of the feed with any search/filter cleared,
                        // instead of just re-flagging a page we're already on.
                        currentPage = HOME;
                        scrollOffset = 0.f;
                        searchedPost = nullptr;
                        searchInput = "";
                        typingSearch = false;
                        selectedPost = nullptr;
                    }

                    if (mouse.x >= 1310 && mouse.x <= 1420 &&
                        mouse.y >= 800 && mouse.y <= 830)
                        currentPage = HELP;

                    if (mouse.x >= 1435 && mouse.x <= 1545 &&
                        mouse.y >= 800 && mouse.y <= 830)
                        currentPage = ABOUT;

                    // NOTIFICATIONS (NEW) -- wide button, full row
                    if (mouse.x >= 1310 && mouse.x <= 1545 &&
                        mouse.y >= 840 && mouse.y <= 872)
                        currentPage = NOTIFICATIONS;

                    Post* temp = feed.head;

                    if (mouse.x >= 850 && mouse.x <= 970 &&
                        mouse.y >= 10 && mouse.y <= 50)
                    {
                        currentPage = SPLASH;
                        splashClock.restart();
                    }

                    // PROFILE (NEW) -- click your name/avatar in the header
                    // to open Edit Profile
                    if (mouse.x >= 1150 && mouse.x <= 1420 &&
                        mouse.y >= 15 && mouse.y <= 65 && currentUser)
                    {
                        openEditProfile();
                    }

                    if (mouse.x >= 1450.f && mouse.x <= 1570.f &&
                        mouse.y >= 20.f && mouse.y <= 60.f)
                    {
                        // LOGOUT -> real logout now, clears currentUser
                        currentPage = LOGIN;
                        loginUsername = "";
                        loginPassword = "";
                        loginErrorMsg = "";
                        activeField = 0;
                        currentUser = nullptr;

                        cout << "Logout clicked!\n";
                    }

                    // FOLLOW button click
                    if (mouse.x >= 1450 && mouse.x <= 1520 &&
                        mouse.y >= 180 && mouse.y <= 210)
                    {
                        if (!friendSuggestions.empty())
                        {
                            if (currentUser)
                                userManager.followUser(currentUser->username, friendSuggestions.front());

                            notifications.push("You started following " + friendSuggestions.front());
                            cout << friendSuggestions.front() << " Followed!" << endl;
                            friendSuggestions.pop();
                        }
                    }

                    int i = 0;

                    while (temp)
                    {
                        float y = 200.f + i * 240.f - scrollOffset;

                        if (y < -160) { temp = temp->next; i++; continue; }
                        if (y > 900)  { temp = temp->next; i++; continue; }

                        // LIKE
                        if (mouse.x >= 840 && mouse.x <= 920 &&
                            mouse.y >= y + 160 && mouse.y <= y + 190)
                        {
                            feed.toggleLike(temp);

                            if (temp->liked)
                                notifications.push("You liked " + temp->user + "'s post");

                            break;
                        }

                        // COMMENT (still a fixed string for now -- typed
                        // comments land in the next increment; this just
                        // proves the Comment linked list works end-to-end)
                        if (mouse.x >= 700 && mouse.x <= 790 &&
                            mouse.y >= y + 160 && mouse.y <= y + 190)
                        {
                            string commenter = currentUser ? currentUser->username : "Guest";
                            feed.addComment(temp, commenter, "Nice Post!");
                            notifications.push("You commented on " + temp->user + "'s post");
                            break;
                        }

                        // SHARE
                        if (mouse.x >= 980 && mouse.x <= 1070 &&
                            mouse.y >= y + 160 && mouse.y <= y + 190)
                        {
                            feed.addShare(temp);
                            notifications.push("You shared " + temp->user + "'s post");
                            break;
                        }

                        // Open post
                        if (mouse.x >= 210 && mouse.x <= 580 &&
                            mouse.y >= y + 45 && mouse.y <= y + 95)
                        {
                            selectedPost = temp;
                            currentPage = POST_VIEW;
                            break;
                        }

                        temp = temp->next;
                        i++;
                    }
                }
            }
        }

        window.clear(sf::Color(230, 235, 255));

        //================ LOGIN PAGE ================
        if (currentPage == LOGIN)
        {
            window.clear(sf::Color(15, 15, 25));

            sf::Text appTitle(font, "Social Media Feed", 50);
            appTitle.setStyle(sf::Text::Bold);
            sf::FloatRect titleBounds = appTitle.getGlobalBounds();
            appTitle.setPosition({(1600.f - titleBounds.size.x) / 2.f, 60.f});
            appTitle.setFillColor(sf::Color::White);
            window.draw(appTitle);

            sf::Sprite logo(instaLogo);
            logo.setScale({0.15f, 0.15f});
            sf::FloatRect logoBounds = logo.getGlobalBounds();
            logo.setPosition({(1600.f - logoBounds.size.x) / 2.f, 100.f});
            window.draw(logo);

            sf::RectangleShape userBox({400.f, 45.f});
            userBox.setPosition({600.f, 370.f});
            userBox.setFillColor(sf::Color::White);
            userBox.setOutlineThickness(activeField == 0 ? 3.f : 1.f);
            userBox.setOutlineColor(activeField == 0 ? sf::Color(24,119,242) : sf::Color(120,120,120));
            window.draw(userBox);

            sf::Text userLabel(font, "Username", 16);
            userLabel.setPosition({600.f, 345.f});
            userLabel.setFillColor(sf::Color::White);
            window.draw(userLabel);

            sf::Text userValue(font, loginUsername.empty() ? "Enter username..." : loginUsername, 20);
            userValue.setPosition({612.f, 382.f});
            userValue.setFillColor(loginUsername.empty() ? sf::Color(150,150,150) : sf::Color::Black);
            window.draw(userValue);

            sf::RectangleShape passBox({400.f, 45.f});
            passBox.setPosition({600.f, 440.f});
            passBox.setFillColor(sf::Color::White);
            passBox.setOutlineThickness(activeField == 1 ? 3.f : 1.f);
            passBox.setOutlineColor(activeField == 1 ? sf::Color(24,119,242) : sf::Color(120,120,120));
            window.draw(passBox);

            sf::Text passLabel(font, "Password", 16);
            passLabel.setPosition({600.f, 415.f});
            passLabel.setFillColor(sf::Color::White);
            window.draw(passLabel);

            string maskedPass = "";
            if (showPassword)
                maskedPass = loginPassword;
            else
                for (size_t k = 0; k < loginPassword.size(); k++)
                    maskedPass += "*";

            sf::Text passValue(font, loginPassword.empty() ? "Enter password..." : maskedPass, 20);
            passValue.setPosition({612.f, 452.f});
            passValue.setFillColor(loginPassword.empty() ? sf::Color(150,150,150) : sf::Color::Black);
            window.draw(passValue);

            sf::Text toggleText(font, showPassword ? "Hide" : "Show", 14);
            toggleText.setPosition({1015.f, 455.f});
            toggleText.setFillColor(sf::Color(24,119,242));
            window.draw(toggleText);

            sf::RectangleShape loginBtn({200.f, 45.f});
            loginBtn.setPosition({700.f, 530.f});
            loginBtn.setFillColor(sf::Color(24,119,242));
            window.draw(loginBtn);

            sf::Text loginBtnText(font, "LOGIN", 20);
            sf::FloatRect btnBounds = loginBtnText.getGlobalBounds();
            loginBtnText.setPosition({700.f + (200.f - btnBounds.size.x) / 2.f, 540.f});
            loginBtnText.setFillColor(sf::Color::White);
            window.draw(loginBtnText);

            // "Create Account" link (NEW) -> Register page
            sf::Text createAccount(font, "New here? Create an Account", 16);
            sf::FloatRect caBounds = createAccount.getGlobalBounds();
            createAccount.setPosition({(1600.f - caBounds.size.x) / 2.f, 610.f});
            createAccount.setFillColor(sf::Color(24,119,242));
            window.draw(createAccount);

            if (!loginErrorMsg.empty())
            {
                sf::Text errText(font, loginErrorMsg, 18);
                sf::FloatRect errBounds = errText.getGlobalBounds();
                errText.setPosition({(1600.f - errBounds.size.x) / 2.f, 655.f});
                errText.setFillColor(sf::Color::Red);
                window.draw(errText);
            }

            sf::Text hint(font, "Demo accounts: Maryam/1234, Ali/1111, Sara/2222", 15);
            sf::FloatRect hintBounds = hint.getGlobalBounds();
            hint.setPosition({(1600.f - hintBounds.size.x) / 2.f, 700.f});
            hint.setFillColor(sf::Color(150,150,150));
            window.draw(hint);

            window.display();
            continue;
        }

        //================ REGISTER PAGE (NEW) ================
        if (currentPage == REGISTER)
        {
            window.clear(sf::Color(15, 15, 25));

            sf::Text title(font, "Create Your Account", 42);
            title.setStyle(sf::Text::Bold);
            sf::FloatRect titleBounds = title.getGlobalBounds();
            title.setPosition({(1600.f - titleBounds.size.x) / 2.f, 60.f});
            title.setFillColor(sf::Color::White);
            window.draw(title);

            auto drawRegField = [&](float boxY, const string& label, const string& value,
                                     bool active, bool mask)
            {
                sf::RectangleShape box({400.f, 45.f});
                box.setPosition({600.f, boxY});
                box.setFillColor(sf::Color::White);
                box.setOutlineThickness(active ? 3.f : 1.f);
                box.setOutlineColor(active ? sf::Color(24,119,242) : sf::Color(120,120,120));
                window.draw(box);

                sf::Text lbl(font, label, 16);
                lbl.setPosition({600.f, boxY - 25.f});
                lbl.setFillColor(sf::Color::White);
                window.draw(lbl);

                string shown = value;
                if (mask)
                {
                    shown = "";
                    for (size_t k = 0; k < value.size(); k++)
                        shown += "*";
                }

                sf::Text val(font, value.empty() ? ("Enter " + label + "...") : shown, 20);
                val.setPosition({612.f, boxY + 12.f});
                val.setFillColor(value.empty() ? sf::Color(150,150,150) : sf::Color::Black);
                window.draw(val);
            };

            drawRegField(320.f, "Username", regUsername, regActiveField == 0, false);
            drawRegField(390.f, "Password", regPassword, regActiveField == 1, true);
            drawRegField(460.f, "Confirm Password", regConfirm, regActiveField == 2, true);

            sf::RectangleShape registerBtn({200.f, 45.f});
            registerBtn.setPosition({700.f, 540.f});
            registerBtn.setFillColor(sf::Color(40,180,99));
            window.draw(registerBtn);

            sf::Text registerBtnText(font, "REGISTER", 20);
            sf::FloatRect rBtnBounds = registerBtnText.getGlobalBounds();
            registerBtnText.setPosition({700.f + (200.f - rBtnBounds.size.x) / 2.f, 550.f});
            registerBtnText.setFillColor(sf::Color::White);
            window.draw(registerBtnText);

            sf::Text backToLogin(font, "Already have an account? Log in", 16);
            sf::FloatRect backBounds = backToLogin.getGlobalBounds();
            backToLogin.setPosition({(1600.f - backBounds.size.x) / 2.f, 615.f});
            backToLogin.setFillColor(sf::Color(24,119,242));
            window.draw(backToLogin);

            if (!regErrorMsg.empty())
            {
                sf::Text errText(font, regErrorMsg, 18);
                sf::FloatRect errBounds = errText.getGlobalBounds();
                errText.setPosition({(1600.f - errBounds.size.x) / 2.f, 660.f});
                errText.setFillColor(sf::Color::Red);
                window.draw(errText);
            }

            if (!regSuccessMsg.empty())
            {
                sf::Text okText(font, regSuccessMsg, 18);
                sf::FloatRect okBounds = okText.getGlobalBounds();
                okText.setPosition({(1600.f - okBounds.size.x) / 2.f, 660.f});
                okText.setFillColor(sf::Color(40,180,99));
                window.draw(okText);
            }

            window.display();
            continue;
        }

        if (currentPage == STORY_VIEW)
        {
            window.clear(sf::Color::Black);
            sf::RectangleShape backBtn({120.f, 40.f});
            backBtn.setPosition({30.f, 30.f});
            backBtn.setFillColor(sf::Color::Red);   // ya Theme::danger

            window.draw(backBtn);

            sf::Text backTxt(font, "BACK", 18);
            backTxt.setPosition({55.f, 38.f});
            backTxt.setFillColor(sf::Color::White);

            window.draw(backTxt);
            sf::Text storyTitle(font, currentStory + "'s Story", 35);
            storyTitle.setPosition({330.f,10.f});
            storyTitle.setFillColor(sf::Color::White);

            sf::Sprite storyPic(*currentStoryTexture);
            if (currentStoryTexture)
                storyPic.setTexture(*currentStoryTexture);
            storyPic.setScale({0.60f,0.60f});
            storyPic.setPosition({200.f,75.f});

            window.draw(storyTitle);
            window.draw(storyPic);

            window.display();
            continue;
        }

        if (currentPage == POST_VIEW && selectedPost != nullptr)
        {
            window.clear(sf::Color(245,245,245));

            sf::Text userName(font, selectedPost->user, 40);
            userName.setPosition({80.f,40.f});
            userName.setFillColor(sf::Color::Black);

            sf::Text postContent(font, selectedPost->content, 32);
            postContent.setPosition({80.f,120.f});
            postContent.setFillColor(sf::Color::Black);

            sf::Sprite postImage(*selectedPost->profilePic);
            postImage.setScale({0.55f,0.55f});
            postImage.setPosition({300.f,180.f});

            window.draw(userName);
            window.draw(postContent);
            window.draw(postImage);

            window.display();
            continue;
        }

        if (currentPage == HELP)
        {
            window.clear(sf::Color(245,245,245));

            sf::Text title(font,"HELP",40);
            title.setPosition({700.f,40.f});
            title.setFillColor(sf::Color::Blue);
            window.draw(title);

            sf::RectangleShape backBtn({120.f,40.f});
            backBtn.setPosition({30.f,30.f});
            backBtn.setFillColor(sf::Color(70,130,180));
            window.draw(backBtn);

            sf::Text back(font,"BACK",18);
            back.setPosition({55.f,40.f});
            back.setFillColor(sf::Color::White);
            window.draw(back);

            sf::Text txt(
                font,
                "Welcome to Social Media Feed\n\n"
                "ADD POST       : Opens Create Post (caption + image)\n\n"
                "DELETE POST    : Delete last post\n\n"
                "SEARCH         : Search a user\n\n"
                "RESET SEARCH   : Remove search highlight\n\n"
                "SORT BY LIKES  : Bubble Sort posts\n\n"
                "LIKE           : Increase likes\n\n"
                "COMMENT        : Add comment\n\n"
                "SHARE          : Share post\n\n"
                "Click your name/avatar (top right) to Edit Profile\n\n"
                "Press ESC to return Home",
                24
            );
            txt.setPosition({180.f,130.f});
            txt.setFillColor(sf::Color::Black);
            window.draw(txt);

            window.display();
            continue;
        }

        if (currentPage == ABOUT)
        {
            window.clear(sf::Color(250,250,250));

            sf::Text title(font,"ABOUT PROJECT",40);
            title.setPosition({570.f,40.f});
            title.setFillColor(sf::Color(0,120,255));
            window.draw(title);

            sf::RectangleShape backBtn({120.f,40.f});
            backBtn.setPosition({30.f,30.f});
            backBtn.setFillColor(sf::Color(70,130,180));
            window.draw(backBtn);

            sf::Text back(font,"BACK",18);
            back.setPosition({55.f,40.f});
            back.setFillColor(sf::Color::White);
            window.draw(back);

            sf::Text info(
                font,
                "SOCIAL MEDIA FEED\n\n"
                "Developed By:\n"
                "Maryam Shumail\nMomina\n\n "
                "BS Computer Science\n"
                "4th Semester\n\n"
                "Data Structures Concepts Used:\n\n"
                "Linked List (Posts, Users, Comments)\n"
                "Queue (Stories, Friend Suggestions)\n"
                "Stack (Page Navigation)\n"
                "Bubble Sort (Sort by Likes)\n\n"
                "GUI Developed Using SFML",
                28
            );
            info.setPosition({350.f,130.f});
            info.setFillColor(sf::Color::Black);
            window.draw(info);

            window.display();
            continue;
        }

        //================ NOTIFICATIONS PAGE (NEW, Increment 2) =================
        if (currentPage == NOTIFICATIONS)
        {
            window.clear(Theme::background);

            sf::RectangleShape backBtn({120.f,40.f});
            backBtn.setPosition({30.f,30.f});
            backBtn.setFillColor(sf::Color::Red);
            window.draw(backBtn);

            sf::Text back(font,"BACK",18);
            back.setPosition({55.f,40.f});
            back.setFillColor(Theme::textOnDark);
            window.draw(back);

            sf::Text title(font,"Notifications",36);
            title.setStyle(sf::Text::Bold);
            title.setPosition({600.f,35.f});
            title.setFillColor(Theme::textPrimary);
            window.draw(title);

            sf::RectangleShape clearBtn({160.f,40.f});
            clearBtn.setPosition({1400.f,30.f});
            clearBtn.setFillColor(Theme::danger);
            window.draw(clearBtn);

            sf::Text clearText(font,"CLEAR ALL",16);
            clearText.setPosition({1432.f,42.f});
            clearText.setFillColor(Theme::textOnDark);
            window.draw(clearText);

            // Walk the Queue front-to-back on a temporary copy so the
            // real queue (the actual DSA store) is never disturbed just
            // by displaying it -- same technique already used for the
            // Stories queue elsewhere in this file.
            queue<string> tempNotifs = notifications;
            float ny = 130.f;

            if (tempNotifs.empty())
            {
                sf::Text empty(font, "No notifications yet -- like, comment, or follow someone!", 20);
                empty.setPosition({80.f, 150.f});
                empty.setFillColor(Theme::textMuted);
                window.draw(empty);
            }

            while (!tempNotifs.empty())
            {
                sf::RectangleShape row({1440.f, 60.f});
                row.setPosition({80.f, ny});
                row.setFillColor(Theme::cardBg);
                row.setOutlineThickness(1.f);
                row.setOutlineColor(Theme::cardBorder);
                window.draw(row);

                sf::CircleShape dot(6.f);
                dot.setPosition({100.f, ny + 27.f});
                dot.setFillColor(Theme::accent);
                window.draw(dot);

                sf::Text line(font, tempNotifs.front(), 18);
                line.setPosition({125.f, ny + 18.f});
                line.setFillColor(Theme::textPrimary);
                window.draw(line);

                tempNotifs.pop();
                ny += 72.f;

                if (ny > 860.f) break;   // stop drawing past the window
            }

            window.display();
            continue;
        }

        //================ EDIT PROFILE PAGE (NEW) =================
        if (currentPage == EDIT_PROFILE)
        {
            window.clear(Theme::background);

            sf::RectangleShape backBtn({120.f,40.f});
            backBtn.setPosition({30.f,30.f});
            backBtn.setFillColor(Theme::neutralDark);
            window.draw(backBtn);

            sf::Text back(font,"BACK",18);
            back.setPosition({55.f,40.f});
            back.setFillColor(Theme::textOnDark);
            window.draw(back);

            sf::Text title(font,"Edit Profile",36);
            title.setStyle(sf::Text::Bold);
            title.setPosition({550.f,35.f});
            title.setFillColor(Theme::textPrimary);
            window.draw(title);

            auto drawField = [&](float y, const string& label, const string& value,
                                  bool active, bool mask)
            {
                sf::Text lbl(font, label, 15);
                lbl.setPosition({550.f, y - 22.f});
                lbl.setFillColor(Theme::textMuted);
                window.draw(lbl);

                sf::RectangleShape box({500.f, 42.f});
                box.setPosition({550.f, y});
                box.setFillColor(Theme::cardBg);
                box.setOutlineThickness(active ? 3.f : 1.f);
                box.setOutlineColor(active ? Theme::accent : Theme::panelBorder);
                window.draw(box);

                string shown = value;
                if (mask)
                {
                    shown = "";
                    for (size_t k = 0; k < value.size(); k++) shown += "*";
                }

                sf::Text val(font, shown, 18);
                val.setPosition({562.f, y + 10.f});
                val.setFillColor(Theme::textPrimary);
                window.draw(val);
            };

            drawField(190.f, "Name", editName, editActiveField == 0, false);
            drawField(255.f, "Bio", editBio, editActiveField == 1, false);

            drawField(320.f, "Current Password", editOldPass, editActiveField == 2, true);
            drawField(385.f, "New Password", editNewPass, editActiveField == 3, true);
            drawField(450.f, "Confirm New Password", editConfirmPass, editActiveField == 4, true);

            sf::RectangleShape saveBtn({200.f, 45.f});
            saveBtn.setPosition({700.f, 610.f});
            saveBtn.setFillColor(Theme::success);
            window.draw(saveBtn);

            sf::Text saveTxt(font, "SAVE", 20);
            sf::FloatRect saveBounds = saveTxt.getGlobalBounds();
            saveTxt.setPosition({700.f + (200.f - saveBounds.size.x) / 2.f, 620.f});
            saveTxt.setFillColor(sf::Color::White);
            window.draw(saveTxt);

            if (!editErrorMsg.empty())
            {
                sf::Text errText(font, editErrorMsg, 18);
                sf::FloatRect eb = errText.getGlobalBounds();
                errText.setPosition({(1600.f - eb.size.x) / 2.f, 715.f});
                errText.setFillColor(Theme::danger);
                window.draw(errText);
            }

            if (!editSuccessMsg.empty())
            {
                sf::Text okText(font, editSuccessMsg, 18);
                sf::FloatRect ob = okText.getGlobalBounds();
                okText.setPosition({(1600.f - ob.size.x) / 2.f, 715.f});
                okText.setFillColor(Theme::success);
                window.draw(okText);
            }

            window.display();
            continue;
        }

        //================ CREATE POST PAGE (NEW) =================
        if (currentPage == CREATE_POST)
        {
            window.clear(Theme::background);

            sf::RectangleShape backBtn({120.f,40.f});
            backBtn.setPosition({30.f,30.f});
            backBtn.setFillColor(Theme::neutralDark);
            window.draw(backBtn);

            sf::Text back(font,"BACK",18);
            back.setPosition({55.f,40.f});
            back.setFillColor(Theme::textOnDark);
            window.draw(back);

            sf::RectangleShape homeBtn({120.f,40.f});
            homeBtn.setPosition({170.f,30.f});
            homeBtn.setFillColor(Theme::neutralDark);
            window.draw(homeBtn);

            sf::Text homeTxt(font,"HOME",18);
            homeTxt.setPosition({195.f,40.f});
            homeTxt.setFillColor(Theme::textOnDark);
            window.draw(homeTxt);

            sf::Text title(font,"Create New Post",36);
            title.setStyle(sf::Text::Bold);
            title.setPosition({550.f,35.f});
            title.setFillColor(Theme::textPrimary);
            window.draw(title);

            sf::Text as(font, "Posting as: " + (currentUser ? currentUser->displayName : "Guest"), 15);
            as.setPosition({550.f, 190.f});
            as.setFillColor(Theme::textMuted);
            window.draw(as);

            sf::Text capLabel(font, "Caption", 15);
            capLabel.setPosition({550.f, 200.f});
            capLabel.setFillColor(Theme::textMuted);
            window.draw(capLabel);

            sf::RectangleShape capBox({750.f, 100.f});
            capBox.setPosition({550.f, 220.f});
            capBox.setFillColor(Theme::cardBg);
            capBox.setOutlineThickness(typingCaption ? 3.f : 1.f);
            capBox.setOutlineColor(typingCaption ? Theme::accent : Theme::panelBorder);
            window.draw(capBox);

            sf::Text capValue(font,
                newPostCaption.empty() ? "What's on your mind?" : newPostCaption, 20);
            capValue.setPosition({562.f, 232.f});
            capValue.setFillColor(newPostCaption.empty() ? Theme::textMuted : Theme::textPrimary);
            window.draw(capValue);

            sf::Text picLabel(font, "Image", 15);
            picLabel.setPosition({550.f, 355.f});
            picLabel.setFillColor(Theme::textMuted);
            window.draw(picLabel);

            sf::Sprite picPreview(*pictureOptions[newPostPicIndex].tex);
            picPreview.setScale({0.25f,0.25f});
            picPreview.setPosition({550.f, 425.f});
            window.draw(picPreview);

            sf::RectangleShape selectImgBtn({210.f, 45.f});
            selectImgBtn.setPosition({550.f, 380.f});
            selectImgBtn.setFillColor(Theme::neutralDark);
            window.draw(selectImgBtn);

            sf::Text selectImgTxt(font, "Select Image: " + pictureOptions[newPostPicIndex].label, 15);
            selectImgTxt.setPosition({562.f, 393.f});
            selectImgTxt.setFillColor(Theme::textOnDark);
            window.draw(selectImgTxt);

            sf::RectangleShape postBtn({200.f,45.f});
            postBtn.setPosition({850.f, 430.f});  // RIGHT
            postBtn.setFillColor(Theme::accent);
            window.draw(postBtn);

            sf::Text postTxt(font,"POST",20);
            sf::FloatRect pb = postTxt.getGlobalBounds();
            postTxt.setPosition({850.f + (200.f - pb.size.x)/2.f, 440.f});
            postTxt.setFillColor(sf::Color::White);
            window.draw(postTxt);

            if (!createPostMsg.empty())
            {
                sf::Text msgText(font, createPostMsg, 17);
                msgText.setPosition({550.f, 545.f});
                msgText.setFillColor(Theme::danger);
                window.draw(msgText);
            }

            window.display();
            continue;
        }

        if (currentPage == SPLASH)
        {
            window.clear(sf::Color(15,15,15));

            sf::Sprite logo(instaLogo);
            logo.setScale({0.18f, 0.18f});
            sf::FloatRect logoBounds = logo.getGlobalBounds();
            logo.setPosition({(1600.f-logoBounds.size.x)/2.f, 150.f});

            sf::Text instaText(font, "Instagram", 55);
            instaText.setFillColor(sf::Color::White);
            sf::FloatRect textBounds = instaText.getGlobalBounds();
            instaText.setPosition({(1600.f-textBounds.size.x)/2.f, 500.f});

            sf::Text loading(font, "Loading...", 22);
            loading.setFillColor(sf::Color(180,180,180));
            sf::FloatRect loadBounds = loading.getGlobalBounds();
            loading.setPosition({(1600.f-loadBounds.size.x)/2.f, 590.f});

            window.draw(logo);
            window.draw(instaText);
            window.draw(loading);

            window.display();
            continue;
        }

        // HEADER (recolored, Increment 2)
        sf::RectangleShape header({1600.f,80.f});
        header.setFillColor(Theme::headerBg);
        window.draw(header);

        sf::Text title(font, "SOCIAL MEDIA FEED (DSA PROJECT)", 28);
        title.setPosition({500.f,10.f});
        title.setFillColor(sf::Color::White);
        window.draw(title);

        // Logged-in-as line -- proves currentUser is real, not fake
        sf::Text loggedInAs(
            font,
            "Logged in as: " + (currentUser ? currentUser->displayName : "Guest"),
            16
        );
        loggedInAs.setPosition({585.f,45.f});
        loggedInAs.setFillColor(sf::Color::White);
        window.draw(loggedInAs);

        // PROFILE (NEW): clickable avatar + name that opens Edit Profile
        if (currentUser)
        {
            sf::Texture* avatarTex = &profileTexture;
            for (auto& opt : pictureOptions)
                if (opt.label == currentUser->profilePicPath)
                    avatarTex = opt.tex;

            sf::CircleShape avatar(24.f);
            avatar.setTexture(avatarTex);
            avatar.setPosition({1160.f, 16.f});
            avatar.setOutlineThickness(2.f);
            avatar.setOutlineColor(Theme::accent);
            window.draw(avatar);

            sf::Text profName(font, currentUser->displayName, 17);
            profName.setPosition({1216.f, 15.f});
            profName.setFillColor(sf::Color::White);
            window.draw(profName);

            sf::Text profHint(font, "Edit Profile", 13);
            profHint.setPosition({1216.f, 40.f});
            profHint.setFillColor(sf::Color(170,170,170));
            window.draw(profHint);
        }

        float storyX = 80.f;
        queue<string> tempStories = stories;

        while (!tempStories.empty())
        {
            sf::CircleShape outerStory(38.f);
            outerStory.setPosition({storyX,85.f});
            outerStory.setFillColor(sf::Color(255,60,150));
            window.draw(outerStory);

            sf::CircleShape whiteBorder(35.f);
            whiteBorder.setPosition({storyX+3.f,88.f});
            whiteBorder.setFillColor(sf::Color::White);
            window.draw(whiteBorder);

            sf::CircleShape story(32.f);
            story.setPosition({storyX+6.f,91.f});

            if (tempStories.front() == "Maryam")
                story.setTexture(&profileTexture);
            else if (tempStories.front() == "Pizza Lover")
                story.setTexture(&pizzaTexture);
            else if (tempStories.front() == "Code Ninja")
                story.setTexture(&profileTexture);
            else if (tempStories.front() == "Coffee Addict")
                story.setTexture(&coffeeTexture);
            else if (tempStories.front() == "Cat Fan")
                story.setTexture(&catTexture);

            window.draw(story);

            sf::Text storyName(font, tempStories.front(), 17);
            storyName.setPosition({storyX - 8.f, 168.f});
            storyName.setFillColor(sf::Color(40,40,40));
            window.draw(storyName);

            tempStories.pop();
            storyX += 110.f;
        }

        Post* temp = feed.head;
        int i = 0;

        while (temp)
        {
            float y = 200.f + i * 240.f - scrollOffset;

            if (y < -170.f) { temp = temp->next; i++; continue; }
            if (y > 900.f)  { temp = temp->next; i++; continue; }

            sf::CircleShape profile(55.f);
            profile.setTexture(temp->profilePic);
            profile.setPosition({90.f, y + 25.f});
            window.draw(profile);

            sf::RectangleShape box({1080.f,220.f});
            box.setPosition({60.f, y});

            if (temp == searchedPost)
                box.setFillColor(Theme::searchTint);
            else if (temp->liked)
                box.setFillColor(Theme::likedTint);
            else
                box.setFillColor(Theme::cardBg);

            box.setOutlineThickness(1.f);
            box.setOutlineColor(Theme::cardBorder);

            sf::Text user(font, temp->user, 20);
            user.setPosition({220.f, y + 30.f});
            user.setFillColor(sf::Color(30,30,30));

            sf::Text content(font, temp->content, 22);
            content.setPosition({220.f, y + 90.f});
            content.setFillColor(sf::Color(60,60,60));

            sf::Text likes(font, "Likes: " + to_string(temp->likes), 18);
            likes.setPosition({930.f, y + 75.f});
            likes.setFillColor(Theme::liked);

            sf::RectangleShape likeButton({80.f, 30.f});
            likeButton.setPosition({840.f, y + 160.f});
            likeButton.setFillColor(temp->liked ? Theme::liked : Theme::unliked);

            sf::Text likeBtnText(font, temp->liked ? "LIKED" : "LIKE", 15);
            likeBtnText.setPosition({temp->liked ? 858.f : 865.f, y + 167.f});
            likeBtnText.setFillColor(temp->liked ? Theme::textOnDark : Theme::textPrimary);

            sf::Text comments(font, "Comments: " + to_string(temp->commentCount), 14);
            comments.setPosition({520.f, y + 15.f});
            comments.setFillColor(Theme::textMuted);

            sf::Text shares(font, "Shares: " + to_string(temp->shares), 14);
            shares.setPosition({520.f, y + 45.f});
            shares.setFillColor(Theme::textMuted);

            // Latest comment now comes from the Comment linked list's tail
            string latestCommentLine = "";
            if (temp->commentTail)
                latestCommentLine = temp->commentTail->username + ": " + temp->commentTail->text;

            sf::Text lastC(font, latestCommentLine, 12);
            lastC.setPosition({220.f, y + 85.f});
            lastC.setFillColor(sf::Color::Black);
            window.draw(lastC);

            window.draw(box);
            window.draw(profile);
            window.draw(user);
            window.draw(content);
            window.draw(likes);
            window.draw(likeButton);
            window.draw(likeBtnText);

            sf::RectangleShape commentButton({90.f, 30.f});
            commentButton.setPosition({700.f, y + 160.f});
            commentButton.setFillColor(Theme::accent);

            sf::Text commentText(font, "COMMENT", 16);
            commentText.setPosition({705.f, y + 166.f});
            commentText.setFillColor(sf::Color::White);

            window.draw(commentButton);
            window.draw(commentText);

            sf::RectangleShape shareButton({90.f, 30.f});
            shareButton.setPosition({980.f, y + 160.f});
            shareButton.setFillColor(Theme::neutralDark);

            sf::Text shareText(font, "SHARE", 14);
            shareText.setPosition({1000.f, y + 167.f});
            shareText.setFillColor(sf::Color::White);

            window.draw(shareButton);
            window.draw(shareText);
            window.draw(comments);
            window.draw(shares);

            temp = temp->next;
            i++;
        }

        //================ POST SCROLL BAR =================
        float listY = 220.f + feed.totalPosts() * 240.f - scrollOffset;

        sf::RectangleShape scrollTrack({10.f,620.f});
        scrollTrack.setPosition({1145.f,200.f});
        scrollTrack.setFillColor(sf::Color(220,220,220));
        window.draw(scrollTrack);

        float thumbHeight = 120.f;
        maxScroll = (feed.totalPosts() * 240.f + 420.f) - 700.f;
        if (maxScroll < 0) maxScroll = 0;
        if (scrollOffset > maxScroll) scrollOffset = maxScroll;

        float thumbY = 200.f;
        if (maxScroll > 0)
            thumbY += (scrollOffset / maxScroll) * (620.f - thumbHeight);

        sf::RectangleShape thumb({10.f,thumbHeight});
        thumb.setPosition({1145.f,thumbY});
        thumb.setFillColor(sf::Color(100,100,100));
        window.draw(thumb);

        //================ SEARCH BOX =================
        //================ SEARCH BOX (moved so Control Panel can't cover it) =================
        sf::RectangleShape searchBox({250.f,32.f});
        searchBox.setPosition({1295.f,460.f});
        searchBox.setFillColor(sf::Color::White);
        searchBox.setOutlineThickness(typingSearch ? 3.f : 1.f);
        searchBox.setOutlineColor(typingSearch ? sf::Color(0,120,255) : sf::Color(150,150,150));
        window.draw(searchBox);

        sf::Text searchBoxValue(
            font,
            searchInput.empty() ? "Search user & press Enter..." : searchInput,
            16
        );
        searchBoxValue.setPosition({1303.f,466.f});
        searchBoxValue.setFillColor(searchInput.empty() ? sf::Color(160,160,160) : sf::Color::Black);
        window.draw(searchBoxValue);

        //================ FRIEND SUGGESTIONS ================
        sf::RectangleShape friendPanel({260.f,340.f});
        friendPanel.setPosition({1290.f,110.f});
        friendPanel.setFillColor(sf::Color::White);
        friendPanel.setOutlineThickness(2.f);
        friendPanel.setOutlineColor(sf::Color(210,210,210));
        window.draw(friendPanel);

        sf::Text friendTitle(font,"Friend Suggestions",20);
        friendTitle.setPosition({1320.f,125.f});
        friendTitle.setFillColor(sf::Color::Black);
        window.draw(friendTitle);

        queue<string> tempFriends = friendSuggestions;
        float fy = 175.f;

        while (!tempFriends.empty())
        {
            sf::CircleShape dp(16);
            dp.setPosition({1320.f,fy});
            dp.setFillColor(sf::Color(70,130,180));
            window.draw(dp);

            sf::Text name(font,tempFriends.front(),18);
            name.setPosition({1360.f,fy});
            name.setFillColor(sf::Color::Black);
            window.draw(name);

            sf::RectangleShape followBtn({65.f,26.f});
            followBtn.setPosition({1460.f,fy});
            followBtn.setFillColor(sf::Color(24,119,242));
            window.draw(followBtn);

            sf::Text follow(font,"Follow",13);
            follow.setPosition({1470.f,fy+4.f});
            follow.setFillColor(sf::Color::White);
            window.draw(follow);

            fy += 55.f;
            tempFriends.pop();
        }

        //================ DASHBOARD =================
        sf::RectangleShape dashboard({260.f,120.f});
        dashboard.setPosition({1290.f,500.f});
        dashboard.setFillColor(sf::Color::White);
        dashboard.setOutlineThickness(2.f);
        dashboard.setOutlineColor(sf::Color(210,210,210));
        window.draw(dashboard);

        sf::Text dashTitle(font,"DASHBOARD",20);
        dashTitle.setPosition({1360.f,510.f});
        dashTitle.setFillColor(sf::Color::Black);
        window.draw(dashTitle);

        sf::Text totalPostsTxt(font, "Posts : " + to_string(feed.totalPosts()), 16);
        totalPostsTxt.setPosition({1320.f,543.f});
        totalPostsTxt.setFillColor(Theme::textPrimary);
        window.draw(totalPostsTxt);

        sf::Text totalLikesTxt(font, "Likes : " + to_string(feed.totalLikes()), 16);
        totalLikesTxt.setPosition({1320.f,563.f});
        totalLikesTxt.setFillColor(Theme::textPrimary);
        window.draw(totalLikesTxt);

        sf::Text totalCommentsTxt(font, "Comments : " + to_string(feed.totalComments()), 16);
        totalCommentsTxt.setPosition({1320.f,583.f});
        totalCommentsTxt.setFillColor(Theme::textPrimary);
        window.draw(totalCommentsTxt);

        sf::Text totalSharesTxt(font, "Shares : " + to_string(feed.totalShares()), 16);
        totalSharesTxt.setPosition({1320.f,603.f});
        totalSharesTxt.setFillColor(Theme::textPrimary);
        window.draw(totalSharesTxt);

        //================ LINKED LIST VISUALIZATION =================
        sf::RectangleShape listPanel({1080.f,120.f});
        listPanel.setPosition({60.f,listY});
        listPanel.setFillColor(sf::Color::White);
        listPanel.setOutlineThickness(2.f);
        listPanel.setOutlineColor(sf::Color(200,200,200));
        window.draw(listPanel);

        sf::Text listTitle(font, "LINKED LIST VISUALIZATION", 18);
        listTitle.setPosition({80.f,listY+10.f});
        listTitle.setFillColor(sf::Color::Black);
        window.draw(listTitle);

        sf::Text headText(font, "HEAD", 20);
        headText.setPosition({90.f, listY + 55.f});
        headText.setFillColor(sf::Color::Red);
        window.draw(headText);

        sf::Vertex headArrow[2];
        headArrow[0].position = sf::Vector2f(150.f, listY + 70.f);
        headArrow[0].color = sf::Color::Black;
        headArrow[1].position = sf::Vector2f(220.f, listY + 70.f);
        headArrow[1].color = sf::Color::Black;
        window.draw(headArrow, 2, sf::PrimitiveType::Lines);

        Post* node = feed.head;
        float nodeX = 220.f;
        int index = 1;

        while (node)
        {
            sf::RectangleShape rect({120.f,60.f});
            rect.setPosition({nodeX,listY+45.f});
            rect.setFillColor(sf::Color(180,220,230));
            rect.setOutlineThickness(2.f);
            rect.setOutlineColor(sf::Color::Black);
            window.draw(rect);

            sf::Text txt(font, "Post " + to_string(index), 16);
            txt.setPosition({nodeX+25.f,listY+65.f});
            txt.setFillColor(sf::Color::Black);
            window.draw(txt);

            if (node->next)
            {
                sf::Vertex line[2];
                line[0].position = sf::Vector2f(nodeX+120.f,listY+75.f);
                line[0].color = sf::Color::Black;
                line[1].position = sf::Vector2f(nodeX+170.f,listY+75.f);
                line[1].color = sf::Color::Black;
                window.draw(line, 2, sf::PrimitiveType::Lines);
            }

            node = node->next;
            nodeX += 170.f;
            index++;
        }

        sf::Text nullText(font, "NULL", 18);
        nullText.setPosition({nodeX + 20.f, listY + 65.f});
        nullText.setFillColor(sf::Color::Blue);
        window.draw(nullText);

        sf::RectangleShape logoutBtn({120.f, 40.f});
        logoutBtn.setPosition({1450.f,20.f});
        logoutBtn.setFillColor(sf::Color(50, 50, 50));

        sf::Text logoutText(font, "LOGOUT", 16);
        logoutText.setPosition({1472.f,27.f});
        logoutText.setFillColor(sf::Color::White);

        window.draw(logoutBtn);
        window.draw(logoutText);

        //================ CONTROL PANEL (recolored, Increment 2) =================
        sf::RectangleShape controlPanel({260.f,260.f});
        controlPanel.setPosition({1290.f,630.f});
        controlPanel.setFillColor(Theme::panelBg);
        controlPanel.setOutlineThickness(1.f);
        controlPanel.setOutlineColor(Theme::panelBorder);
        window.draw(controlPanel);

        sf::Text controlTitle(font,"CONTROLS",20);
        controlTitle.setStyle(sf::Text::Bold);
        controlTitle.setPosition({1360.f,640.f});
        controlTitle.setFillColor(Theme::textPrimary);
        window.draw(controlTitle);

        // Small helper-drawn buttons all share one accent language now:
        // primary actions = accent blue, destructive = muted danger red,
        // everything else = neutral charcoal. No more rainbow of colors.
        auto drawPanelBtn = [&](float x, float y, float w, const string& label, sf::Color fill)
        {
            sf::RectangleShape btn({w, 30.f});
            btn.setPosition({x, y});
            btn.setFillColor(fill);
            window.draw(btn);

            sf::Text txt(font, label, 14);
            sf::FloatRect tb = txt.getGlobalBounds();
            txt.setPosition({x + (w - tb.size.x) / 2.f - tb.position.x, y + 7.f});
            txt.setFillColor(Theme::textOnDark);
            window.draw(txt);
        };

        drawPanelBtn(1310.f, 680.f, 110.f, "ADD",     Theme::accent);
        drawPanelBtn(1435.f, 680.f, 110.f, "DELETE",  Theme::danger);
        drawPanelBtn(1310.f, 720.f, 110.f, "SEARCH",  Theme::neutralDark);
        drawPanelBtn(1435.f, 720.f, 110.f, "RESET",   Theme::neutralDark);
        drawPanelBtn(1310.f, 760.f, 110.f, "SORT",    Theme::accent);
        drawPanelBtn(1435.f, 760.f, 110.f, "HOME",    Theme::neutralDark);
        drawPanelBtn(1310.f, 800.f, 110.f, "HELP",    Theme::neutralDark);
        drawPanelBtn(1435.f, 800.f, 110.f, "ABOUT",   Theme::neutralDark);
        drawPanelBtn(
            1310.f, 840.f, 235.f,
            notifications.empty()
                ? "NOTIFICATIONS"
                : "NOTIFICATIONS (" + to_string(notifications.size()) + ")",
            Theme::accent
        );

        window.display();
    }

    return 0;
}
