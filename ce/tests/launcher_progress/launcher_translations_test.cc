#include "launcher_translations.h"
#include <cstdio>
#include <cstdlib>

using namespace launcher_translations;

static void check(bool ok)
{
    if (!ok) { std::puts("translation catalog test failed"); std::exit(1); }
}

int main()
{
    check(find("1c") && find("fargus") && find("levy") && !find("old-rp"));
    check(available("original", true, "", "") && available("official", true, "", ""));
    check(available("1c", false, "https://example.org/1c", "https://example.org/fonts"));
    check(available("levy", false, "http://127.0.0.1:8080/levy", "http://localhost:8080/fonts"));
    check(!available("1c", true, "https://example.org/1c", "https://example.org/fonts"));
    check(!available("1c", false, "", "https://example.org/fonts"));
    check(!available("1c", false, "http://example.org/1c", "https://example.org/fonts"));
    check(!validUrl("http://127.0.0.1:80" "@example.org/1c"));
    check(!available("unknown", false, "https://example.org/1", "https://example.org/fonts"));
}
