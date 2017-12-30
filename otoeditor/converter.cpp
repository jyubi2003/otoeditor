#include "stdafx.h"
#include "otoeditor.h"

std::wstring multi_to_wide_capi(std::string const& src)
{
    std::size_t converted{};
    std::vector<wchar_t> dest(src.size(), L'\0');
    if (::_mbstowcs_s_l(&converted, dest.data(), dest.size(), src.data(), _TRUNCATE, ::_create_locale(LC_ALL, "jpn")) != 0) {
        throw std::system_error{errno, std::system_category()};
    }
    return std::wstring(dest.begin(), dest.end());
}

std::string wide_to_multi_capi(std::wstring const& src)
{
    std::size_t converted{};
    std::vector<char> dest(src.size() * sizeof(wchar_t) + 1 , '\0');
    if (::_wcstombs_s_l(&converted, dest.data(), dest.size(), src.data(), _TRUNCATE, ::_create_locale(LC_ALL, "jpn")) != 0) {
        throw std::system_error{errno, std::system_category()};
    }
    return std::string(dest.begin(), dest.end());
}

std::wstring multi_to_wide_winapi(std::string const& src)
{
    auto const dest_size = ::MultiByteToWideChar(CP_ACP, 0U, src.data(), -1, nullptr, 0U);
    std::vector<wchar_t> dest(dest_size, L'\0');
    if (::MultiByteToWideChar(CP_ACP, 0U, src.data(), -1, dest.data(), dest.size()) == 0) {
        throw std::system_error{static_cast<int>(::GetLastError()), std::system_category()};
    }
    return std::wstring(dest.begin(), dest.end());
}

std::string wide_to_multi_winapi(std::wstring const& src)
{
    auto const dest_size = ::WideCharToMultiByte(CP_ACP, 0U, src.data(), -1, nullptr, 0, nullptr, nullptr);
    std::vector<char> dest(dest_size, '\0');
    if (::WideCharToMultiByte(CP_ACP, 0U, src.data(), -1, dest.data(), dest.size(), nullptr, nullptr) == 0) {
        throw std::system_error{static_cast<int>(::GetLastError()), std::system_category()};
    }
    return std::string(dest.begin(), dest.end());
}

