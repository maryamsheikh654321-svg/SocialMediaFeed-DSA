#include "Post.h"

Post::Post(string u, string c, sf::Texture* pic)
{
    user = u;
    content = c;
    likes = 0;
    liked = false;
    shares = 0;
    profilePic = pic;
    commentHead = nullptr;
    commentTail = nullptr;
    commentCount = 0;
    next = nullptr;
}

void Feed::addPost(string user, string content, sf::Texture* pic)
{
    Post* newPost = new Post(user, content, pic);

    if (!head)
    {
        head = newPost;
        return;
    }

    Post* temp = head;
    while (temp->next)
        temp = temp->next;

    temp->next = newPost;
}

void Feed::deleteLastPost()
{
    if (!head) return;

    if (!head->next)
    {
        delete head;
        head = nullptr;
        return;
    }

    Post* temp = head;

    while (temp->next->next)
        temp = temp->next;

    delete temp->next;
    temp->next = nullptr;
}

void Feed::toggleLike(Post* p)
{
    if (!p) return;

    p->likes++;

    if (p->likes % 2 == 1)
        p->liked = true;
    else
        p->liked = false;
}

void Feed::addComment(Post* p, string username, string text)
{
    if (!p) return;

    Comment* newComment = new Comment(username, text);

    if (!p->commentHead)
    {
        p->commentHead = newComment;
        p->commentTail = newComment;
    }
    else
    {
        p->commentTail->next = newComment;   // O(1) append
        p->commentTail = newComment;
    }

    p->commentCount++;
}

void Feed::addShare(Post* p)
{
    if (p)
        p->shares++;
}

int Feed::totalPosts()
{
    int count = 0;
    Post* temp = head;

    while (temp)
    {
        count++;
        temp = temp->next;
    }

    return count;
}

int Feed::totalLikes()
{
    int total = 0;
    Post* temp = head;

    while (temp)
    {
        total += temp->likes;
        temp = temp->next;
    }

    return total;
}

int Feed::totalComments()
{
    int total = 0;
    Post* temp = head;

    while (temp)
    {
        total += temp->commentCount;
        temp = temp->next;
    }

    return total;
}

int Feed::totalShares()
{
    int total = 0;
    Post* temp = head;

    while (temp)
    {
        total += temp->shares;
        temp = temp->next;
    }

    return total;
}

void Feed::sortByLikes()
{
    if (head == nullptr)
        return;

    bool swapped;

    do
    {
        swapped = false;
        Post* current = head;

        while (current->next)
        {
            if (current->likes < current->next->likes)
            {
                swap(current->user, current->next->user);
                swap(current->content, current->next->content);
                swap(current->likes, current->next->likes);
                swap(current->liked, current->next->liked);
                swap(current->shares, current->next->shares);
                swap(current->profilePic, current->next->profilePic);
                swap(current->commentHead, current->next->commentHead);
                swap(current->commentTail, current->next->commentTail);
                swap(current->commentCount, current->next->commentCount);

                swapped = true;
            }

            current = current->next;
        }

    } while (swapped);
}

Post* Feed::searchPost(string name)
{
    Post* temp = head;

    while (temp)
    {
        if (temp->user == name)
            return temp;

        temp = temp->next;
    }

    return nullptr;
}
