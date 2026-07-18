#ifndef POST_H
#define POST_H

#include <string>
#include <SFML/Graphics.hpp>

using namespace std;

// ==================================================================
// Comment
// ------------------------------------------------------------------
// DSA: Singly Linked List (one list per post)
// WHY: A post can receive an unknown, growing number of comments.
//      Appending a comment should be O(1), and we display them
//      oldest-first, which a linked list does naturally by walking
//      head -> tail. This replaces the old single "lastComment"
//      string field, which could only ever remember the newest one.
// ==================================================================
struct Comment
{
    string username;
    string text;
    Comment* next;

    Comment(string u, string t) : username(u), text(t), next(nullptr) {}
};

// ==================================================================
// Post
// ------------------------------------------------------------------
// Unchanged core idea from your original project: each Post is a
// node in Feed's Singly Linked List. Added: a Comment linked list
// (commentHead/commentTail) instead of a single lastComment string.
// ==================================================================
struct Post
{
    string user;
    string content;
    int likes;
    bool liked;
    int shares;

    sf::Texture* profilePic;

    Comment* commentHead;
    Comment* commentTail;
    int commentCount;

    Post* next;

    Post(string u, string c, sf::Texture* pic);
};

// ==================================================================
// Feed
// ------------------------------------------------------------------
// DSA: Singly Linked List
// WHY: Posts are added/removed from one end frequently (new posts,
//      undo-delete) and the whole feed is displayed by a single
//      head-to-tail walk -- exactly what a linked list is good at,
//      without the shifting cost of a vector::insert(begin()).
//
// Time complexity summary:
//   addPost        -> O(n) to reach the tail (no tail pointer yet --
//                      kept from your original design; can be
//                      upgraded to O(1) later if needed)
//   deleteLastPost -> O(n) (must find the second-to-last node)
//   toggleLike/addComment/addShare -> O(1) given a Post*
//   totalPosts/totalLikes/totalComments/totalShares -> O(n)
//   sortByLikes    -> Bubble Sort, O(n^2) worst case, O(n) best case
//                      (already-sorted early exit via `swapped` flag)
//   searchPost     -> Linear Search, O(n)
// ==================================================================
class Feed
{
public:
    Post* head = nullptr;

    void addPost(string user, string content, sf::Texture* pic);
    void deleteLastPost();

    void toggleLike(Post* p);
    void addComment(Post* p, string username, string text);
    void addShare(Post* p);

    int totalPosts();
    int totalLikes();
    int totalComments();
    int totalShares();

    void sortByLikes();
    Post* searchPost(string name);
};

#endif
