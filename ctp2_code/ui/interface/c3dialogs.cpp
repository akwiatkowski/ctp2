#include "ctp/c3.h"

#include <memory>

#include "ui/aui_ctp2/c3_utilitydialogbox.h"
#include "ui/interface/c3dialogs.h"

#include "gs/gameobj/player.h"






ForeignTradeBidInfo::ForeignTradeBidInfo(sint32 player, Unit &fromCity,
											Unit &toCity, sint32 resource)
											:m_player(player),
											 m_fromCity(fromCity),
											 m_toCity(toCity),
											 m_resource(resource)
{
}

void c3dialogs_PostForeignTradeBidDialog(sint32 player, Unit &fromCity, Unit &toCity,
								sint32 resource)
{
	std::unique_ptr<ForeignTradeBidInfo> info =
		std::make_unique<ForeignTradeBidInfo>(player, fromCity, toCity, resource);




	static std::unique_ptr<c3_UtilityTextFieldPopup> pop;
	if(!pop) {
		pop = std::make_unique<c3_UtilityTextFieldPopup>(
			c3dialogs_ForeignTradeBidDialogCallback,
			nullptr,
			nullptr,
			nullptr,
			"ForeignTradeBidPopup",
			(void *)info.get());
	}
	pop->m_data = info.release();
	pop->DisplayWindow();
}

void c3dialogs_ForeignTradeBidDialogCallback(MBCHAR const *text, sint32 accepted, void *data)
{
	std::unique_ptr<ForeignTradeBidInfo> info(static_cast<ForeignTradeBidInfo *>(data));

	if (accepted) {
		sint32 price = atoi(text);
		player_Get(info->m_player)->SendTradeBid(info->m_fromCity, info->m_resource,
												info->m_toCity, price);
	}
}
