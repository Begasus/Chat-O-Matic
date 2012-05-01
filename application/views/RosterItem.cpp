/*
 * Copyright 2009, Andrea Anzani. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Andrea Anzani, andrea.anzani@gmail.com
 *		Pier Luigi Fiorini, pierluigi.fiorini@gmail.com
 */

#include <ListView.h>

#include <libinterface/BitmapUtils.h>

#include "CayaUtils.h"
#include "CayaResources.h"
#include "ContactLinker.h"
#include "NotifyMessage.h"
#include "RosterItem.h"


RosterItem::RosterItem(const char*  name, ContactLinker* contact)
	: BStringItem(name),
	fBitmap(NULL),
	fStatus(CAYA_OFFLINE),
	contactLinker(contact),
	fVisible(true)
{
	rgb_color highlightColor = ui_color(B_CONTROL_HIGHLIGHT_COLOR);
	rgb_color darkenHighlightColor = tint_color(highlightColor, B_DARKEN_1_TINT);

	fGradient.AddColor(highlightColor, 0);
	fGradient.AddColor(darkenHighlightColor, 255);	
}


RosterItem::~RosterItem()
{
	delete fBitmap;
}


void
RosterItem::SetVisible(bool visible)
{
	fVisible = visible;
}


void	
RosterItem::SetBitmap(BBitmap* bitmap)
{
	if (fBitmap != NULL)
		delete fBitmap;
	fBitmap = bitmap;
}


void 
RosterItem::ObserveString(int32 what, BString str)
{
	switch (what) {
		case STR_CONTACT_NAME:
			SetText(str);
			break;
		case STR_PERSONAL_STATUS:
			SetPersonalStatus(str);
			break;
	}
}


void 
RosterItem::ObservePointer(int32 what, void* ptr)
{
	switch (what) {
		case PTR_AVATAR_BITMAP:
			SetBitmap((BBitmap*)ptr);
			break;
	}
}


void 
RosterItem::ObserveInteger(int32 what, int32 val)
{
	switch (what) {
		case INT_CONTACT_STATUS:
			SetStatus((CayaStatus)val);
			break;
	}
}


void RosterItem::DrawItem(BView* owner, BRect frame, bool complete)
{
	if (Text() == NULL)
	       return;

	rgb_color highlightColor = ui_color(B_CONTROL_HIGHLIGHT_COLOR);
	rgb_color highColor = owner->HighColor();
	rgb_color lowColor = owner->LowColor();

	// Draw selection
	if (IsSelected()) {
		fGradient.SetStart(frame.LeftTop());
		fGradient.SetEnd(frame.LeftBottom());
		owner->SetLowColor(highlightColor);
		owner->FillRect(frame, fGradient);
	} else if (complete) {
		owner->SetHighColor(lowColor);
		owner->FillRect(frame);
	}

	BResources* res = CayaResources();
	if (res) {
		int32 num = 0;

		switch (fStatus) {
			case CAYA_ONLINE:
				num = kOnlineIcon;
				break;
			case CAYA_EXTENDED_AWAY:
			case CAYA_AWAY:
				num = kAwayIcon;
				break;
			case CAYA_DO_NOT_DISTURB:
				num = kBusyIcon;
				break;
			case CAYA_OFFLINE:
				num = kOfflineIcon;
				break;
			default:
				break;
		}

		BBitmap* bitmap = IconFromResources(res, num, B_MINI_ICON);
		BRect bitmapRect(frame.left + 2, frame.top + 2,
			frame.left + 2 + 14, frame.top + 2 + 14);

		owner->SetDrawingMode(B_OP_ALPHA);
		owner->SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_OVERLAY);
		owner->DrawBitmap(bitmap, bitmap->Bounds(),
			bitmapRect, B_FILTER_BITMAP_BILINEAR);

		delete res;
	} else {
		// Draw contact status
		switch (fStatus) {
			case CAYA_ONLINE:
				owner->SetHighColor(CAYA_GREEN_COLOR);
				break;
			case CAYA_EXTENDED_AWAY:
			case CAYA_AWAY:
				owner->SetHighColor(CAYA_ORANGE_COLOR);
				break;
			case CAYA_DO_NOT_DISTURB:
				owner->SetHighColor(CAYA_RED_COLOR);
				break;
			case CAYA_OFFLINE:
				break;
			default:
		   		break;
		}

		owner->FillEllipse(BRect(frame.left + 4, frame.top + 4,
			frame.left + 4 + 10 , frame.top + 4 + 10));
	}

	// Draw contact name
	owner->MovePenTo(frame.left + 20, frame.top + fBaselineOffset);
	owner->SetHighColor(ui_color(B_CONTROL_TEXT_COLOR));
	owner->DrawString(Text());

	// Draw contact status string
	owner->MovePenTo(frame.left + 20, frame.top + fBaselineOffset +
		fBaselineOffset + 2);
	owner->SetHighColor(tint_color(lowColor, B_DARKEN_1_TINT));
	if (fPersonalStatus.Length() == 0)
		owner->DrawString(CayaStatusToString(fStatus));
	else
		owner->DrawString(fPersonalStatus);

	// Draw separator between items
	owner->StrokeLine(BPoint(frame.left, frame.bottom),
		BPoint(frame.right, frame.bottom));

	// Draw avatar icon
	if (fBitmap != NULL) {
		float h = frame.Height() - 4;
		BRect rect(frame.right - h - 2, frame.top + 2,
			frame.right - 2, frame.top + h );
		owner->SetDrawingMode(B_OP_ALPHA);
		owner->SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_OVERLAY);
		owner->DrawBitmap(fBitmap, fBitmap->Bounds(),
			rect, B_FILTER_BITMAP_BILINEAR);
	}

	BBitmap* protocolBitmap = contactLinker->ProtocolBitmap();
	float h = frame.Height();
	BRect rect(frame.right - h - 20, frame.top + 2,
			frame.right - 40, frame.top + h - 20);
	owner->SetDrawingMode(B_OP_ALPHA);
	owner->SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_OVERLAY);
	owner->DrawBitmap(protocolBitmap, protocolBitmap->Bounds(),
		rect, B_FILTER_BITMAP_BILINEAR);

	owner->SetHighColor(highColor);
	owner->SetLowColor(lowColor);
}


void
RosterItem::Update(BView* owner, const BFont* font)
{
	font_height fheight;
	font->GetHeight(&fheight);

	fBaselineOffset = 2 + ceilf(fheight.ascent + fheight.leading / 2);

	SetHeight((ceilf(fheight.ascent) + ceilf(fheight.descent) +
		ceilf(fheight.leading) + 4 ) * 2);
}


void
RosterItem::SetStatus(CayaStatus status)
{
	if (fStatus != status)
		fStatus = status;
}
