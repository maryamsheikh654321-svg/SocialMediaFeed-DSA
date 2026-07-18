#ifndef USER_H
#define USER_H

#include <string>
#include <vector>

using namespace std;

// ==================================================================
// User
// ------------------------------------------------------------------
// Each registered user is one node in a Singly Linked List (see
// UserManager below). A vector is used only for the small, per-user
// followers/following lists -- that is NOT the "central" DSA for
// this feature, it's just bookkeeping on a node.
// ==================================================================
struct User
{
    string username;
    string password;
    string displayName;      // editable "Name" shown on posts/profile (Edit Profile)
    string bio;
    string profilePicPath;   // key used to pick a texture in Main.cpp (Edit Profile)

    vector<string> followers;  // usernames that follow this user
    vector<string> following;  // usernames this user follows

    int postsCount;

    User* next;   // singly linked list pointer

    User(string uname, string pass);
};

// ==================================================================
// UserManager
// ------------------------------------------------------------------
// DSA: Singly Linked List (+ a tail pointer for O(1) append)
//
// WHY a linked list and not a vector/array here:
//   - Users are created dynamically (registration happens at
//     runtime, count is unknown ahead of time).
//   - We never need random access by index, only "find by username"
//     and "walk everyone" -- both are natural linked-list operations.
//   - Insertion at the tail is O(1) with the tail pointer, and no
//     resize/copy ever happens like it would with a growing vector.
//
// Time complexity summary:
//   registerUser   -> O(n) to check for a duplicate username,
//                      O(1) to append once it's confirmed unique
//   authenticate   -> O(n) linear search (same pattern as
//                      Feed::searchPost, applied to users)
//   findUser       -> O(n) linear search
//   searchUsers    -> O(n) linear scan, substring match
//   followUser     -> O(1) amortized vector push_back
//
// PERSISTENCE (NEW):
//   Accounts previously lived only in this in-memory linked list, so
//   every registered account disappeared when the program closed --
//   only seedDemoUsers() accounts (Maryam/Ali/Sara/...) survived a
//   restart. saveUsers()/loadUsers() persist the linked list to a
//   plain text file (users.txt) so registered accounts (and any
//   profile/password edits) survive across runs.
// ==================================================================
class UserManager
{
public:
    User* head;
    User* tail;
    int userCount;

    UserManager();

    bool registerUser(string uname, string pass);
    User* authenticate(string uname, string pass);
    User* findUser(string uname);
    vector<User*> searchUsers(string query);

    bool followUser(string followerName, string targetName);
    bool unfollowUser(string followerName, string targetName);
    bool isFollowing(string followerName, string targetName);

    void seedDemoUsers();   // Maryam/1234, Ali/1111, Sara/2222, ...

    // NEW: persistence -- walks the linked list and writes one line
    // per user to a text file; loadUsers() reads that file back in
    // and re-creates (or updates) the matching linked-list nodes.
    void saveUsers(const string& filename = "users.txt");
    void loadUsers(const string& filename = "users.txt");
};

#endif
