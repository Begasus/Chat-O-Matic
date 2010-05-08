/*
 * Copyright 2009, Pier Luigi Fiorini. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Pier Luigi Fiorini, pierluigi.fiorini@gmail.com
 */

#include <Bitmap.h>
#include <GroupLayout.h>
#include <GroupLayoutBuilder.h>
#include <Message.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <PopUpMenu.h>

#include <libinterface/BitmapMenuItem.h>
#include <libinterface/BitmapUtils.h>

#include "AccountManager.h"
#include "BitmapView.h"
#include "CayaUtils.h"
#include "NicknameTextControl.h"
#include "StatusMenuItem.h"
#include "StatusView.h"

const int32 kSetNickname = 'NICH';


StatusView::StatusView(const char* name)
	: BView(name, B_WILL_DRAW)
{
	// Nick name
	fNickname = new NicknameTextControl("nickname", new BMessage(kSetNickname));

	// Status menu
	fStatusMenu = new BPopUpMenu("-");

	// Add status menu items
	int32 s = CAYA_ONLINE;
	while (s >= CAYA_ONLINE && s < CAYA_STATUSES) {
		StatusMenuItem* item = new StatusMenuItem(CayaStatusToString(
			(CayaStatus)s), (CayaStatus)s);
		fStatusMenu->AddItem(item);

		// Add items for custom messages
		if (s == CAYA_ONLINE || s == CAYA_DO_NOT_DISTURB) {
			item = new StatusMenuItem("Custom...", (CayaStatus)s, true);
			fStatusMenu->AddItem(item);
			fStatusMenu->AddItem(new BSeparatorItem());
		}

		// Mark offline status by default
		if (s == CAYA_OFFLINE)
			item->SetMarked(true);

		s++;
	}

	// Menu field
	BMenuField* statusField = new BMenuField("statusField", NULL,
		fStatusMenu, NULL);

	// Icon
	fAvatar = new BitmapView("icon");

	// Set layout
	SetLayout(new BGroupLayout(B_VERTICAL));
	AddChild(BGroupLayoutBuilder(B_HORIZONTAL, 5)
		.AddGroup(B_VERTICAL)
			.Add(fNickname)
			.Add(statusField)
			.AddGlue()
		.End()
		.Add(fAvatar)
	);
}


void
StatusView::AttachedToWindow()
{
	fNickname->SetTarget(this);
	fStatusMenu->SetTargetForItems(this);
}


void
StatusView::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
		case kSetNickname:
		{
			AccountManager* accountManager = AccountManager::Get();
			accountManager->SetNickname(fNickname->Text());
			break;
		}
		case kSetStatus:
		{
			int32 status;

			if (msg->FindInt32("status", &status) != B_OK)
				return;

			AccountManager* accountManager = AccountManager::Get();
			accountManager->SetStatus((CayaStatus)status, "");
			break;
		}
		default:
			BView::MessageReceived(msg);
	}
}


void	
StatusView::SetName(BString name)
{
	fNickname->SetText(name.String());	
}


void
StatusView::SetStatus(CayaStatus status)
{
	for (int32 i = 0; i < fStatusMenu->CountItems(); i++) {
		StatusMenuItem* item
			= reinterpret_cast<StatusMenuItem*>(fStatusMenu->ItemAt(i));
		if (item && item->Status() == status && !item->IsCustom())
			item->SetMarked(true);
	}
}


void
StatusView::SetAvatar(BBitmap* bitmap)
{
	BBitmap* b = RescaleBitmap(bitmap, 49, 49);
	fAvatar->SetBitmap(b);
}