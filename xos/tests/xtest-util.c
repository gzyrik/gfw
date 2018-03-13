#include <xos.h>
void uri_test()
{
    xint    i;
    xUri    *uri;
    xstr    str;
    xcstr   uri_list[]={
        "http://www.w3.org/albert/bertram/marie%2Dclaude",
        "http://www.w3.org/albert/bert%3Fram/marie-claude",
        "http://www.w3.org/albert/bertram%2Fmarie-claude",
        "file:/c:/a%20b%20%20b?type=animal&name=narwhal#nose",
        "http://example.org/absolute/URI/with/absolute/path/to/resource.txt",
        "ftp://example.org/resource.txt",
        "foo://username:password@example.com:8042/over/there/index.dtb?type=animal&name=narwhal#nose",
        "urn:example:animal:ferret:nose",
        "mailto:username@example.com?subject=Topic",
        "urn:issn:1535-3613",
        "http://en.wikipedia.org/wiki/URI#Examples_of_URI_references",
        "//example.org/scheme-relative/URI/with/absolute/path/to/resource.txt",
        "/relative/URI/with/absolute/path/to/resource.txt",
        "relative/path/to/resource.txt",
        "../../../resource.txt",
        "./resource.txt#frag01",
        "resource.txt",
        "#frag01",
        "",
        NULL
    };
    for (i =0; uri_list[i]; ++i) {
        uri = x_uri_parse (uri_list[i],NULL);
        str = x_uri_build (uri,FALSE);
        x_printf("\n%s\n%s\n",uri_list[i], str);
        x_free(str);
        x_free (uri);
    }
}
