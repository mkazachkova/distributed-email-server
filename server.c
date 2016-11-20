#include "net_include.h"


/**********RECEIVING************/
//CASE: received type 4 update (new user has been created)
//take name off of the update
//iterate through your user array and see if that user exists
//if not, create a new user object with the username from the update
//append update to the corresponding index to the machine that sent it (append to linked list)
//(do something with updating the counter)


//CASE: received type 1 update (new email)
//first, check if that email already exists. If it does, just store the update
//if email doesn't exist, take to field off of email and find that user in your user linked list
//if user doesn't exist, create new user using the to field (****double check this, might just display an error)
//if user to does exist, find that object is user linked list and access the email linked list of that user
//using the timestamps insert the email into the correct location for that email
//append update to the corresponding index to the machine that sent it (append to linked list)
//(do something with counter maybe)


//CASE: receive type 2 update (read)
//first, check if user whose mailbox you'll be modifiying actually exists or not
//if user doesn't exist, create new user object with name of user. Then, create a new node in the linked list of emails for that user
//with null data. Set the read bool equal to true for that specific email
//If user does exist, loop through linked list of email to find the email with that specific time stamp
//if email doesn't exist then create a new node and insert it into the linked list where the email should have been.
//set the data for this node to null, and set the bool for read to true.
//If email does exist, set the the read bool to true. If already true, then can set it to true again without anything going wrong


//CASE: receive type 3 update (delete)
//first, check if user whose mailbox you'll be modifiying actually exists or not
//if user doesn't exist, create new user object with name of user. Then, create a new node in the linked list of emails for that user
//with null data. Set the delete bool equal to true for that specific email
//If user does exist, loop through linked list of email to find the email with that specific time stamp
//if email doesn't exist then create a new node and insert it into the linked list where the email should have been.
//set the data for this node to null, and set the bool for delete to true.
//If email does exist, set the the delete bool to true. If already true, then can set it to true again without anything going wrong















/**********CLIENT FUNCTIONS**********/

//CASE: client login to server (new client)
//Client and server are put into private group (membership and spread stuff)
//Server x (server that user is on) creates a new user object and appends it to user linked list
//Server x sends all other servers (multicast) in its partition an update saying a new user has joined


//CASE: client login to server (returning client)
//Client and server in private group
//Server x iterates through user linked list and finds user object. No update is sent and no new user object is created


//CASE: client switches servers
//private group between cliet and original server is dropped
//private group between client and new server is created


//CASE: client wants to display all headers
//find specific use in linked list of users
//iterate through email linked list within the user and display (backwards) all emails with non-null data


//CASE: client sends an email
//client is prompted for to, subject, and message.
//Server x (the server the user is currently on) multicasts an update with type 1 signifiying a new email (first create email
//and update object; attach timestamp as needed)

//CASE: client wants to delete a message
//client selects email to delete after displaying the headers (deleting yth email)
//iterate from tail to find yth email; take time stamp off of this email and store as t. Change data in email to null
//note for above: need to iterate over non-null data
//server x (the server the user is currently on) sends an update (type 3) with a timestamp_of_email (this will be t) and its
//own timestamp as well 


//CASE: client reads an email
//client selects an email to read. The message of the email is diplayed to the client (iterate through linked list from tail
//and that the xth email ignoring null data). Server x sends an update (type 2) with the timestamp of the email that was read
//along with its own timestamp and multicasts it out

//CASE: client wants to print memberships
//use spread to print all servers currently in the partition




/**********WHEN MEMBERSHIP CHANGES**********/
//use anti entropy. boom. done

//we attach a 1d array with what we (as a server) know from every other server to every update that we send. When a server receives this
//they add it to their 2d array of what all servers know from all other servers.

//When there is a merge each server involved in the merge multicasts out a struct (merge_matrix) containing its own 2d matrix.

//When you receive one of these merge_matrix structs you iterate through it and compare each cell to the corresponding cell
//in your own 2d array. You take the max of the values and stre that as the cell in your 2d array. You must wait until you receive however many
//servers there are in the new partition worth of these merge_matrix structs. Ignore the merge_matrix struct you receive from yourself.

//once your 2d array is loaded with all of the new information, you must only consider the servers that you are in a partition with
//currently as possible sources of information (this is the vertical side of the array). You look at each column and find the max value (only
//considering the processes that are in the partition). The process with the max value is responsible for sending the updates to all other processes
//in the partition
