#include "User.h"
#include <algorithm>
#include <fstream>
#include <sstream>
#include <iostream>

User::User(string uname, string pass)
{
    username = uname;
    password = pass;
    displayName = uname;
    bio = "Living life one post at a time.";
    profilePicPath = "";
    postsCount = 0;
    next = nullptr;
}

UserManager::UserManager()
{
    head = nullptr;
    tail = nullptr;
    userCount = 0;
}

bool UserManager::registerUser(string uname, string pass)
{
    if (uname.empty() || pass.empty())
        return false;

    // O(n) duplicate check -- linear search, same pattern used
    // throughout the project for "search by name".
    if (findUser(uname) != nullptr)
        return false;

    User* newUser = new User(uname, pass);

    if (!head)
    {
        head = newUser;
        tail = newUser;
    }
    else
    {
        tail->next = newUser;   // O(1) append thanks to tail pointer
        tail = newUser;
    }

    userCount++;
    return true;
}

User* UserManager::authenticate(string uname, string pass)
{
    User* temp = head;

    while (temp)
    {
        if (temp->username == uname && temp->password == pass)
            return temp;

        temp = temp->next;
    }

    return nullptr;
}

User* UserManager::findUser(string uname)
{
    // Case-insensitive match: usernames are typed fresh every login
    // (fields are cleared on logout/restart), so a small capitalization
    // slip -- "Test" at registration vs "test" at login -- shouldn't
    // ever look like "no such account". Usernames are effectively
    // case-insensitive identifiers here.
    string target = uname;
    transform(target.begin(), target.end(), target.begin(), ::tolower);

    User* temp = head;

    while (temp)
    {
        string current = temp->username;
        transform(current.begin(), current.end(), current.begin(), ::tolower);

        if (current == target)
            return temp;

        temp = temp->next;
    }

    return nullptr;
}

vector<User*> UserManager::searchUsers(string query)
{
    vector<User*> results;

    if (query.empty())
        return results;

    string lowerQuery = query;
    transform(lowerQuery.begin(), lowerQuery.end(), lowerQuery.begin(), ::tolower);

    User* temp = head;

    while (temp)
    {
        string lowerName = temp->username;
        transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);

        if (lowerName.find(lowerQuery) != string::npos)
            results.push_back(temp);

        temp = temp->next;
    }

    return results;
}

bool UserManager::isFollowing(string followerName, string targetName)
{
    User* follower = findUser(followerName);
    if (!follower) return false;

    for (auto& f : follower->following)
        if (f == targetName)
            return true;

    return false;
}

bool UserManager::followUser(string followerName, string targetName)
{
    if (followerName == targetName)
        return false;

    User* follower = findUser(followerName);
    User* target = findUser(targetName);

    if (!follower || !target)
        return false;

    if (isFollowing(followerName, targetName))
        return false;

    follower->following.push_back(targetName);
    target->followers.push_back(followerName);

    return true;
}

bool UserManager::unfollowUser(string followerName, string targetName)
{
    User* follower = findUser(followerName);
    User* target = findUser(targetName);

    if (!follower || !target)
        return false;

    auto& followingList = follower->following;
    followingList.erase(
        remove(followingList.begin(), followingList.end(), targetName),
        followingList.end()
    );

    auto& followerList = target->followers;
    followerList.erase(
        remove(followerList.begin(), followerList.end(), followerName),
        followerList.end()
    );

    return true;
}

void UserManager::seedDemoUsers()
{
    registerUser("Maryam", "1234");
    registerUser("Ali", "1111");
    registerUser("Sara", "2222");
    registerUser("Ahmad", "3333");
    registerUser("Hifza", "4444");
    registerUser("Momina", "5555");

    // A couple of demo follow relationships so the Followers/Following
    // counts aren't all zero on first run.
    followUser("Ali", "Maryam");
    followUser("Sara", "Maryam");
    followUser("Maryam", "Ali");
}

// ==================================================================
// PERSISTENCE (NEW)
// ------------------------------------------------------------------
// Accounts only ever lived in this run's linked list, so restarting
// the program lost every registered account -- only the hard-coded
// seedDemoUsers() accounts came back. These two functions save the
// linked list to a plain text file (one user per line, fields
// separated by '|') and load it back in on the next run.
//
// File line format:
//   username|password|displayName|bio|profilePicPath
//
// NOTE: bio/displayName are expected not to contain '|' or newlines
// (Main.cpp's text-entry fields only accept normal typed characters,
// so this holds in practice for this project).
// ==================================================================

void UserManager::saveUsers(const string& filename)
{
    ofstream file(filename);

    if (!file.is_open())
    {
        cout << "Could not open " << filename << " for saving users!\n";
        return;
    }

    User* temp = head;

    while (temp)
    {
        file << temp->username      << "|"
             << temp->password      << "|"
             << temp->displayName   << "|"
             << temp->bio           << "|"
             << temp->profilePicPath
             << "\n";

        temp = temp->next;
    }

    file.close();
}

void UserManager::loadUsers(const string& filename)
{
    ifstream file(filename);

    if (!file.is_open())
        return;   // no saved users yet -- not an error, just first run

    string line;

    while (getline(file, line))
    {
        if (line.empty())
            continue;

        stringstream ss(line);
        string uname, pass, dispName, bio, picPath;

        getline(ss, uname,   '|');
        getline(ss, pass,    '|');
        getline(ss, dispName,'|');
        getline(ss, bio,     '|');
        getline(ss, picPath, '|');

        if (uname.empty())
            continue;

        User* existing = findUser(uname);

        if (existing)
        {
            // Already in the list (e.g. a seeded demo account) --
            // update it with whatever was last saved (covers a demo
            // account whose password/profile was edited and saved).
            existing->password       = pass;
            existing->displayName    = dispName.empty() ? uname : dispName;
            existing->bio            = bio;
            existing->profilePicPath = picPath;
        }
        else
        {
            if (registerUser(uname, pass))
            {
                User* newUser = findUser(uname);
                if (newUser)
                {
                    newUser->displayName    = dispName.empty() ? uname : dispName;
                    newUser->bio            = bio;
                    newUser->profilePicPath = picPath;
                }
            }
        }
    }

    file.close();
}
