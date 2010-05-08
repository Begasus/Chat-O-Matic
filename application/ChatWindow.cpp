/*
 * Copyright 2009, Andrea Anzani. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Andrea Anzani, andrea.anzani@gmail.com
 */

#include <Alert.h>
#include <Application.h>
#include <Box.h>
#include <Button.h>
#include <CheckBox.h>
#include <GridLayout.h>
#include <GridLayoutBuilder.h>
#include <GroupLayout.h>
#include <GroupLayoutBuilder.h>
#include <Layout.h>
#include <ListView.h>
#include <Message.h>
#include <SpaceLayoutItem.h>
#include <ScrollView.h>
#include <String.h>

#include "ChatWindow.h"
#include "ContactLinker.h"
#include "EditingFilter.h"
#include "CayaConstants.h"
#include "CayaRenderView.h"
#include "NotifyMessage.h"

const int32 kCloseWindow = 'clwn';
const int32 kChat = 'chat';


ChatWindow::ChatWindow(ContactLinker* cl)
	: BWindow(BRect(200, 200, 500, 500),
		cl->GetName().String(), B_DOCUMENT_WINDOW, 0),
	fContactLinker(cl)
{
	fReceiveView = new CayaRenderView("fReceiveView");
	fReceiveView->SetOtherNick(cl->GetName());
	BScrollView* scrollViewReceive = new BScrollView("scrollviewR", fReceiveView,
		B_WILL_DRAW, false, true);

	fSendView = new BTextView("fReceiveView");
	BScrollView* scrollViewSend = new BScrollView("scrollviewS", fSendView,
		B_WILL_DRAW, false, true);	
	fSendView->SetWordWrap(true);
	AddCommonFilter(new EditingFilter(fSendView));
	fSendView->MakeFocus(true);

	SetLayout(new BGroupLayout(B_HORIZONTAL));

	AddChild(BGroupLayoutBuilder(B_VERTICAL, 10)
		.Add(scrollViewReceive, 2)
		.Add(scrollViewSend)
		.SetInsets(5, 5, 5, 5)
	);	

	MoveTo(BAlert::AlertPosition(Bounds().Width(), Bounds().Height() / 2));

	fSendView->MakeFocus(true);
	
}


bool
ChatWindow::QuitRequested()
{
	BMessage msg(kCloseWindow);
	msg.AddString("id", fContactLinker->GetId());
	fContactLinker->GetMessenger().SendMessage(&msg);
	return false;
}


void
ChatWindow::MessageReceived(BMessage* message)
{
	switch(message->what) {
		case kChat:
		{
			BString message = fSendView->Text();
			if (message == "")
				return;

			fReceiveView->AppendOwnMessage(message.String());

			BMessage msg(IM_MESSAGE);
			msg.AddInt32("im_what", IM_SEND_MESSAGE);
			msg.AddString("id", fContactLinker->GetId());
			msg.AddString("message", message);
			fContactLinker->GetMessenger().SendMessage(&msg);

			fSendView->SetText("");
			break;
		}
		case IM_MESSAGE:
			ImMessage(message);
			break;
		default:
			BWindow::MessageReceived(message);
			break;
	}	
}


void
ChatWindow::ImMessage(BMessage* msg)
{
	int32 im_what = msg->FindInt32("im_what");
	switch (im_what) {
		case IM_MESSAGE_RECEIVED:
		{
			BString message = msg->FindString("message");
			fReceiveView->AppendOtherMessage(message.String());
			break;
		}		
		case IM_STATUS_CHANGED:
		{
#if 0
			int32 status;

			if (msg->FindInt32("status", &status) != B_OK)
				return;

			BString id = msg->FindString("id");

			if (id != "" && status >= CAYA_ONLINE) {
				bool found = false;
				BStringItem *item = fRosterMap.ValueFor(id,&found);

				if (!found) {
					item = new BStringItem(id.String());
					item->SetHeight(50.0);
					listView->AddItem(item);
					fRosterMap.AddItem(id, item);
				} else {
					bool itemPresent = listView->HasItem(item);
					if (status == CAYA_OFFLINE) {
						//remove from list. (for now)
						if (itemPresent)
							listView->RemoveItem(item);
					} else {
						if(!itemPresent)
							listView->AddItem(item);
					}
				}

				UpdateListItem(item);
			}
#endif
			break;
		}
		case IM_CONTACT_INFO:
		{
#if 0
			BString id = msg->FindString("id");
			BString fullName = msg->FindString("nick");
			if (id != "" && fullName != "") {
				bool found = false;
				BStringItem *item = fRosterMap.ValueFor(id,&found);
				if (!found) {
					item = new BStringItem(id.String());
					item->SetHeight(50.0);
					fRosterMap.AddItem(id, item);
				}

				item->SetText(fullName);
				UpdateListItem(item);
			}
#endif
			break;
		}
		default:
			break;
	}
}

void 
ChatWindow::ObserveString(int32 what, BString str)
{
	switch(what) {
		case STR_CONTACT_NAME:
			if (Lock()){
				SetTitle(str);
				fReceiveView->SetOtherNick(str);
				Unlock();
			}
			break;
		case STR_PERSONAL_STATUS:
			break;
	}
}


void 
ChatWindow::ObservePointer(int32 what, void* ptr)
{
	switch (what) {
		case PTR_AVATAR_BITMAP:			
			break;
	}
}


void 
ChatWindow::ObserveInteger(int32 what, int32 val)
{
	switch (what) {
		case INT_CONTACT_STATUS:
			if (Lock()) {
				AppendStatus((CayaStatus)val);
				Unlock();
			}
			break;
	}
}


void	
ChatWindow::AppendStatus(CayaStatus status)
{	
	BString message(fContactLinker->GetName());

    switch (status) {
		case CAYA_ONLINE:
			message << " is available";
   			break;
	   	case CAYA_EXTENDED_AWAY:   		
	   	case CAYA_AWAY:
			message << " is away";
	   		break;
	   	case CAYA_DO_NOT_DISTURB:
			message << " is busy, please do not disturb!";
	   		break;
	   	case CAYA_OFFLINE:	    
			message << " is offline";			
	   		break;
		default:
	   		break;
	}

	fReceiveView->Append(message.String(), COL_TEXT, COL_TEXT, R_TEXT); 
 	fReceiveView->Append("\n", COL_TEXT, COL_TEXT, R_TEXT);
	fReceiveView->ScrollToSelection();
}