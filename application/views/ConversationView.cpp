/*
 * Copyright 2009-2011, Andrea Anzani. All rights reserved.
 * Copyright 2021, Jaidyn Levesque. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Andrea Anzani, andrea.anzani@gmail.com
 *		Jaidyn Levesque, jadedctrl@teknik.io
 */

#include "ConversationView.h"

#include <LayoutBuilder.h>
#include <ListView.h>
#include <ScrollView.h>
#include <StringList.h>
#include <StringView.h>

#include <libinterface/BitmapView.h>

#include "AppMessages.h"
#include "AppPreferences.h"
#include "ChatProtocolMessages.h"
#include "Conversation.h"
#include "NotifyMessage.h"
#include "RenderView.h"
#include "SendTextView.h"
#include "User.h"
#include "UserItem.h"
#include "UserListView.h"
#include "Utils.h"


ConversationView::ConversationView()
	:
	BGroupView("chatView", B_VERTICAL, B_USE_DEFAULT_SPACING),
	fMessageQueue(),
	fConversation(NULL)
{
	_InitInterface();
	fSendView->MakeFocus(true);
}


ConversationView::ConversationView(Conversation* chat)
	:
	#if defined(__i386__) && !defined(__x86_64__)
	BGroupView("chatView", B_VERTICAL, B_USE_DEFAULT_SPACING),
	fMessageQueue()
	#else
	ConversationView()
	#endif
{
	SetConversation(chat);
	fUserList->SetConversation(chat);
}


bool
ConversationView::QuitRequested()
{
	BMessage msg(APP_CLOSE_CHAT_WINDOW);
	msg.AddString("chat_id", fConversation->GetId());
	fConversation->Messenger().SendMessage(&msg);
	return false;
}


void
ConversationView::AttachedToWindow()
{
	while (fMessageQueue.IsEmpty() == false) {
		BMessage* msg = fMessageQueue.RemoveItemAt(0);
		ImMessage(msg);
	}
	if (fConversation != NULL) {
		if (fNameTextView->Text() != fConversation->GetName())
			fNameTextView->SetText(fConversation->GetName());
		if (fSubjectTextView->Text() != fConversation->GetSubject())
			fSubjectTextView->SetText(fConversation->GetSubject());
	}
	NotifyInteger(INT_WINDOW_FOCUSED, 0);
}


void
ConversationView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case APP_CHAT:
		{
			BString text = fSendView->Text();
			if (text == "")
				return;
			int64 instance = fConversation->GetProtocolLooper()->GetInstance();

			BMessage msg(IM_MESSAGE);
			msg.AddInt32("im_what", IM_SEND_MESSAGE);
			msg.AddInt64("instance", instance);
			msg.AddString("chat_id", fConversation->GetId());
			msg.AddString("body", text);
			fConversation->ImMessage(&msg);

			fSendView->SetText("");
			break;
		}
		case IM_MESSAGE:
			ImMessage(message);
			break;

		default:
			BGroupView::MessageReceived(message);
			break;
	}
}


void
ConversationView::ImMessage(BMessage* msg)
{
	int32 im_what = msg->FindInt32("im_what");

	switch (im_what) {
		case IM_ROOM_LEFT:
		{
			delete fConversation;
			delete this;
			break;
		}
		case IM_MESSAGE_RECEIVED:
		{
			_AppendOrEnqueueMessage(msg);
			fReceiveView->ScrollToBottom();
			break;
		}
		case IM_MESSAGE_SENT:
		case IM_LOGS_RECEIVED:
		{
			if (im_what == IM_MESSAGE_SENT)
				fReceiveView->ScrollToBottom();
			_AppendOrEnqueueMessage(msg);
			break;
		}
		case IM_ROOM_JOINED:
		{
			BMessage msg;
			msg.AddString("body", "** You joined the room.\n");
			_AppendOrEnqueueMessage(&msg);
		}
		case IM_ROOM_CREATED:
		{
			BMessage msg;
			msg.AddString("body", "** You created the room.\n");
			_AppendOrEnqueueMessage(&msg);
		}
		case IM_ROOM_PARTICIPANT_JOINED:
		{
			_UserMessage("%user% has joined the room.\n",
						 "%user% has joined the room (%body%).\n", msg);
			break;
		}
		case IM_ROOM_PARTICIPANT_LEFT:
		{
			_UserMessage("%user% has left the room.\n",
						 "%user% has left the room (%body%).\n", msg);
			break;
		}
		case IM_ROOM_PARTICIPANT_KICKED:
		{
			_UserMessage("%user% was kicked.\n",
						 "%user% was kicked (%body%).\n", msg);
			break;
		}
		case IM_ROOM_PARTICIPANT_BANNED:
		{
			_UserMessage("%user% has been banned.\n",
						 "%user% has been banned (%body%).\n", msg);
			break;
		}
		default:
			break;
	}
}


Conversation*
ConversationView::GetConversation()
{
	return fConversation;
}


void
ConversationView::SetConversation(Conversation* chat)
{
	if (chat == NULL)
		return;
	fConversation =  chat;
	fNameTextView->SetText(chat->GetName());
	fSubjectTextView->SetText(chat->GetSubject());
	fProtocolView->SetBitmap(chat->ProtocolBitmap());
}


void
ConversationView::UpdateIcon()
{
	if (fConversation->IconBitmap() != NULL)
		fIcon->SetBitmap(fConversation->IconBitmap());
}


void
ConversationView::UpdateUserList(UserMap users)
{
	fUserList->MakeEmpty();
	for (int i = 0; i < users.CountItems(); i++) {
		User* user = users.ValueAt(i);
		if (fUserList->HasItem(user->GetListItem()) == false)
			fUserList->AddItem(user->GetListItem());
	}
}


void
ConversationView::InvalidateUserList()
{
	for (int i = 0; i < fUserList->CountItems(); i++)
		fUserList->InvalidateItem(i);
}


void
ConversationView::ObserveString(int32 what, BString str)
{
	switch (what)
	{
		case STR_ROOM_NAME:
		{
			fNameTextView->SetText(str);
			break;
		}
		case STR_ROOM_SUBJECT:
		{
			fSubjectTextView->SetText(str);
			break;
		}
	}
}


void
ConversationView::_InitInterface()
{
	fReceiveView = new RenderView("receiveView");
	BScrollView* scrollViewReceive = new BScrollView("receiveScrollView",
		fReceiveView, B_WILL_DRAW, false, true);

	fSendView = new SendTextView("sendView", this);

	fNameTextView = new BTextView("roomName", B_WILL_DRAW);
	fNameTextView->SetViewUIColor(B_PANEL_BACKGROUND_COLOR);
	fNameTextView->MakeEditable(false);

	fSubjectTextView = new BTextView("roomSubject", B_WILL_DRAW);
	fSubjectTextView->SetViewUIColor(B_PANEL_BACKGROUND_COLOR);
	fSubjectTextView->MakeEditable(false);

	fIcon = new BitmapView("ContactIcon");
	fIcon->SetExplicitMinSize(BSize(50, 50));
	fIcon->SetExplicitPreferredSize(BSize(50, 50));
	fIcon->SetExplicitAlignment(BAlignment(B_ALIGN_RIGHT, B_ALIGN_MIDDLE));

	fProtocolView = new BitmapView("protocolView");

	fUserList = new UserListView("userList");
	BScrollView* scrollViewUsers = new BScrollView("userScrollView",
		fUserList, B_WILL_DRAW, false, true);

	BLayoutBuilder::Group<>(this, B_VERTICAL)
		.AddGroup(B_HORIZONTAL)
			.Add(fIcon)
			.AddGroup(B_VERTICAL)
				.Add(fNameTextView)
				.Add(fSubjectTextView)
			.End()
			.Add(fProtocolView)
		.End()
		.AddSplit(B_HORIZONTAL, 0)
			.AddGroup(B_VERTICAL, B_USE_HALF_ITEM_SPACING, 8)
				.Add(scrollViewReceive, 20)
				.Add(fSendView, 1)
			.End()
			.Add(scrollViewUsers, 1)
		.End()
	.End();
}


bool
ConversationView::_AppendOrEnqueueMessage(BMessage* msg)
{
	// If not attached to the chat window, then re-handle this message
	// later [AttachedToWindow()], since you can't edit an unattached 
	// RenderView.
	if (Window() == NULL) {
		fMessageQueue.AddItem(new BMessage(*msg));
		return false;
	}

	// Alright, we're good to append!
	_AppendMessage(msg);
	return true;
}


void
ConversationView::_AppendMessage(BMessage* msg)
{
	BStringList users, bodies;
	if (msg->FindStrings("body", &bodies) != B_OK)
		return;
	msg->FindStrings("user_id", &users);

	for (int i = bodies.CountStrings(); i >= 0; i--) {
		User* sender = fConversation->UserById(users.StringAt(i));
		BString sender_name = users.StringAt(i);
		BString body = bodies.StringAt(i);
		rgb_color userColor = ui_color(B_PANEL_TEXT_COLOR);
		int64 timeInt;

		if (msg->FindInt64("when", i, &timeInt) != B_OK)
			timeInt = (int64)time(NULL);

		if (sender != NULL) {
			sender_name = sender->GetName();
			userColor = sender->fItemColor;
		}

		if (sender_name.IsEmpty() == true) {
			fReceiveView->AppendGenericMessage(body.String());
			continue;
		}

		fReceiveView->AppendMessage(sender_name.String(), body.String(),
									userColor, (time_t)timeInt);
	}
}


void
ConversationView::_UserMessage(const char* format, const char* bodyFormat,
							   BMessage* msg)
{
	BString user_id;
	BString user_name = msg->FindString("user_name");
	BString body = msg->FindString("body");

	if (msg->FindString("user_id", &user_id) != B_OK)
		return;
	if (user_name.IsEmpty() == true)
		user_name = user_id;

	BString newBody("** ");
	if (body.IsEmpty() == true)
		newBody << format;
	else {
		newBody << bodyFormat;
		newBody.ReplaceAll("%body%", body.String());
	}
	newBody.ReplaceAll("%user%", user_name.String());

	BMessage newMsg;
	newMsg.AddString("body", newBody);
	_AppendOrEnqueueMessage(&newMsg);
}


