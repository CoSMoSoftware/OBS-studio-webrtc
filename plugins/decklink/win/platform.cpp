#include "../platform.hpp"

#include <util/platform.h>

IDeckLinkDiscovery *CreateDeckLinkDiscoveryInstance(void)
{
	IDeckLinkDiscovery *instance;
	const HRESULT result =
		CoCreateInstance(CLSID_CDeckLinkDiscovery, nullptr, CLSCTX_ALL,
				 IID_IDeckLinkDiscovery, (void **)&instance);
	return result == S_OK ? instance : nullptr;
}

bool DeckLinkStringToStdString(decklink_string_t input, std::string &output)
{
	if (input == nullptr)
		return false;

	size_t len = wcslen(input);
	size_t utf8_len = os_wcs_to_utf8(input, len, nullptr, 0);

	output.resize(utf8_len);
	os_wcs_to_utf8(input, len, &output[0], utf8_len);

	return true;
}
