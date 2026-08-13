#ifndef OKINAWA_DOCS_DOCSGEN_HPP
#define OKINAWA_DOCS_DOCSGEN_HPP

#include <string>
#include <vector>

namespace docsgen {

  struct FrontMatter {
    std::string title;
    std::string section;
    int         navOrder;
    bool        hasNavOrder;
  };

  struct Page {
    std::string srcRelPath;  // e.g. "reference/items.md"
    std::string outRelPath;  // e.g. "reference/items.html"
    std::string title;
    std::string section;
    int         navOrder;
    bool        hasNavOrder;
    std::string body;  // markdown body (front-matter stripped)
  };

  // Split a leading "---\n ... \n---\n" front-matter block. Returns true only
  // when a complete block (opening and closing "---" lines) is present, in
  // which case fmOut holds the parsed keys (title, section, nav_order) and
  // bodyOut is the text after the closing delimiter. On false, fmOut is
  // unspecified and bodyOut == content. Tolerates a UTF-8 BOM and CRLF line
  // endings.
  bool parseFrontMatter(const std::string &content, FrontMatter &fmOut,
                        std::string &bodyOut);

  // Map a source markdown path to its output html path:
  //   "index.md" -> "index.html", "reference/items.md" ->
  //   "reference/items.html".
  std::string outputPathFor(const std::string &srcRelPath);

  // Sidebar ordering rank for a section. Lower sorts first.
  // Start(0), Reference(1), Examples(2); anything else -> 1000.
  int sectionRank(const std::string &section);

  // Build sidebar nav HTML from all pages, grouped by section (ranked, then
  // alphabetical), ordered by navOrder within a section. The home page
  // (outRelPath == "index.html") is excluded. The page whose outRelPath ==
  // currentOutPath gets class="active". Links are root-relative ("/foo.html");
  // applyBaseUrl prefixes them afterwards.
  std::string buildNav(const std::vector<Page> &pages,
                       const std::string       &currentOutPath);

  // Prefix every root-relative href="/..." and src="/..." with baseUrl.
  // Leaves href="#...", protocol-relative "//...", and external "http..."
  // alone. baseUrl "" returns html unchanged.
  std::string applyBaseUrl(const std::string &html, const std::string &baseUrl);

  // Replace all occurrences of {{title}}, {{nav}}, {{content}}, {{base_url}}.
  std::string renderTemplate(const std::string &tmpl, const std::string &title,
                             const std::string &nav, const std::string &content,
                             const std::string &baseUrl);

  // Render a Markdown fragment to HTML via md4c (GitHub dialect: tables,
  // strikethrough, permissive autolinks, task lists).
  std::string renderMarkdown(const std::string &markdown);

}  // namespace docsgen

#endif  // OKINAWA_DOCS_DOCSGEN_HPP
