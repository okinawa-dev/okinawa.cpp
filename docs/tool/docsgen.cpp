#include "docsgen.hpp"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <map>

#include <md4c-html.h>

namespace {

  std::string trim(const std::string &s) {
    std::size_t a = 0;
    std::size_t b = s.size();
    while (a < b && (s[a] == ' ' || s[a] == '\t'))
      a++;
    while (b > a && (s[b - 1] == ' ' || s[b - 1] == '\t' || s[b - 1] == '\r'))
      b--;
    return s.substr(a, b - a);
  }

  std::string htmlEscape(const std::string &s) {
    std::string out;
    out.reserve(s.size());
    for (std::size_t i = 0; i < s.size(); i++) {
      char c = s[i];
      if (c == '&') {
        out += "&amp;";
      } else if (c == '<') {
        out += "&lt;";
      } else if (c == '>') {
        out += "&gt;";
      } else if (c == '"') {
        out += "&quot;";
      } else {
        out += c;
      }
    }
    return out;
  }

  bool pageLess(const docsgen::Page &a, const docsgen::Page &b) {
    int ra = docsgen::sectionRank(a.section);
    int rb = docsgen::sectionRank(b.section);
    if (ra != rb)
      return ra < rb;
    if (a.section != b.section)
      return a.section < b.section;
    if (a.navOrder != b.navOrder)
      return a.navOrder < b.navOrder;
    return a.title < b.title;
  }

  std::string replaceAll(std::string s, const std::string &from,
                         const std::string &to) {
    if (from.empty())
      return s;
    std::size_t pos = 0;
    while ((pos = s.find(from, pos)) != std::string::npos) {
      s.replace(pos, from.size(), to);
      pos += to.size();
    }
    return s;
  }

  // Insert baseUrl before the leading '/' of every `token + "/..."` whose value
  // is root-relative (single leading slash, not "//").
  std::string prefixRootRel(const std::string &html, const std::string &token,
                            const std::string &baseUrl) {
    std::string out;
    out.reserve(html.size() + 64);
    std::size_t i = 0;
    while (i < html.size()) {
      std::size_t hit = html.find(token, i);
      if (hit == std::string::npos) {
        out += html.substr(i);
        break;
      }
      out += html.substr(i, hit - i);
      out += token;
      std::size_t after = hit + token.size();
      if (after < html.size() && html[after] == '/' &&
          !(after + 1 < html.size() && html[after + 1] == '/')) {
        out += baseUrl;
      }
      i = after;
    }
    return out;
  }

  void mdSink(const MD_CHAR *text, MD_SIZE size, void *userdata) {
    std::string *out = static_cast<std::string *>(userdata);
    out->append(text, size);
  }

  // Strip HTML tags from a fragment, leaving its text content.
  std::string stripTags(const std::string &html) {
    std::string out;
    bool        inTag = false;
    for (std::size_t i = 0; i < html.size(); i++) {
      char c = html[i];
      if (c == '<') {
        inTag = true;
      } else if (c == '>') {
        inTag = false;
      } else if (!inTag) {
        out += c;
      }
    }
    return out;
  }

  // GitHub-style heading slug: lowercase, runs of non-alphanumeric become a
  // single hyphen, leading/trailing hyphens trimmed.
  std::string slugify(const std::string &text) {
    std::string out;
    bool        pendingHyphen = false;
    for (std::size_t i = 0; i < text.size(); i++) {
      unsigned char c = static_cast<unsigned char>(text[i]);
      if (std::isalnum(c)) {
        if (pendingHyphen && !out.empty())
          out += '-';
        pendingHyphen = false;
        out += static_cast<char>(std::tolower(c));
      } else {
        pendingHyphen = true;
      }
    }
    return out;
  }

  // Give every <h1>..<h6> an id slugged from its text, so sections can be
  // deep-linked (e.g. handlers.html#oktexturehandler). Duplicate slugs on a
  // page get a -2, -3, ... suffix.
  std::string addHeadingAnchors(const std::string &html) {
    std::string out;
    out.reserve(html.size() + 128);
    std::map<std::string, int> seen;
    std::size_t                i = 0;
    while (i < html.size()) {
      if (html[i] == '<' && i + 3 < html.size() && html[i + 1] == 'h' &&
          html[i + 2] >= '1' && html[i + 2] <= '6' && html[i + 3] == '>') {
        char        level    = html[i + 2];
        std::string closeTag = std::string("</h") + level + ">";
        std::size_t start    = i + 4;
        std::size_t close    = html.find(closeTag, start);
        if (close != std::string::npos) {
          std::string inner = html.substr(start, close - start);
          std::string slug  = slugify(stripTags(inner));
          if (!slug.empty()) {
            int         n  = ++seen[slug];
            std::string id = (n == 1) ? slug : slug + "-" + std::to_string(n);
            out += "<h";
            out += level;
            out += " id=\"" + id + "\">";
            out += inner;
            out += closeTag;
            i = close + closeTag.size();
            continue;
          }
        }
      }
      out += html[i++];
    }
    return out;
  }

}  // namespace

namespace docsgen {

  bool parseFrontMatter(const std::string &content, FrontMatter &fmOut,
                        std::string &bodyOut) {
    fmOut             = FrontMatter();
    fmOut.navOrder    = 0;
    fmOut.hasNavOrder = false;

    std::size_t pos = 0;
    if (content.compare(0, 3, "\xEF\xBB\xBF") == 0)
      pos = 3;  // skip BOM

    std::size_t firstNl = content.find('\n', pos);
    if (firstNl == std::string::npos) {
      bodyOut = content;
      return false;
    }
    std::string firstLine = trim(content.substr(pos, firstNl - pos));
    if (firstLine != "---") {
      bodyOut = content;
      return false;
    }

    std::size_t lineStart = firstNl + 1;
    while (lineStart <= content.size()) {
      std::size_t nl   = content.find('\n', lineStart);
      std::string line = (nl == std::string::npos)
                             ? content.substr(lineStart)
                             : content.substr(lineStart, nl - lineStart);
      if (trim(line) == "---") {
        bodyOut = (nl == std::string::npos) ? "" : content.substr(nl + 1);
        return true;
      }
      std::size_t colon = line.find(':');
      if (colon != std::string::npos) {
        std::string key = trim(line.substr(0, colon));
        std::string val = trim(line.substr(colon + 1));
        if (key == "title") {
          fmOut.title = val;
        } else if (key == "section") {
          fmOut.section = val;
        } else if (key == "nav_order") {
          fmOut.navOrder    = std::atoi(val.c_str());
          fmOut.hasNavOrder = true;
        }
      }
      if (nl == std::string::npos)
        break;
      lineStart = nl + 1;
    }

    bodyOut = content;  // no closing delimiter -> not a valid block
    return false;
  }

  std::string outputPathFor(const std::string &srcRelPath) {
    const std::string ext = ".md";
    if (srcRelPath.size() >= ext.size() &&
        srcRelPath.compare(srcRelPath.size() - ext.size(), ext.size(), ext) ==
            0) {
      return srcRelPath.substr(0, srcRelPath.size() - ext.size()) + ".html";
    }
    return srcRelPath;
  }

  int sectionRank(const std::string &section) {
    if (section == "Start")
      return 0;
    if (section == "Reference")
      return 1;
    if (section == "Examples")
      return 2;
    return 1000;
  }

  std::string buildNav(const std::vector<Page> &pages,
                       const std::string       &currentOutPath) {
    std::vector<Page> items;
    for (std::size_t i = 0; i < pages.size(); i++) {
      if (pages[i].outRelPath == "index.html")
        continue;
      items.push_back(pages[i]);
    }
    std::stable_sort(items.begin(), items.end(), pageLess);

    std::string html;
    std::string currentSection;
    bool        open = false;
    for (std::size_t i = 0; i < items.size(); i++) {
      const Page &p = items[i];
      if (i == 0 || p.section != currentSection) {
        if (open)
          html += "</ul>\n";
        currentSection = p.section;
        html += "<div class=\"nav-section\">" + htmlEscape(p.section) +
                "</div>\n<ul>\n";
        open = true;
      }
      std::string cls =
          (p.outRelPath == currentOutPath) ? " class=\"active\"" : "";
      html += "<li><a href=\"/" + p.outRelPath + "\"" + cls + ">" +
              htmlEscape(p.title) + "</a></li>\n";
    }
    if (open)
      html += "</ul>\n";
    return html;
  }

  std::string applyBaseUrl(const std::string &html,
                           const std::string &baseUrl) {
    if (baseUrl.empty())
      return html;
    std::string out = prefixRootRel(html, "href=\"", baseUrl);
    out             = prefixRootRel(out, "src=\"", baseUrl);
    return out;
  }

  std::string renderTemplate(const std::string &tmpl, const std::string &title,
                             const std::string &nav, const std::string &content,
                             const std::string &baseUrl) {
    std::string out = tmpl;
    out             = replaceAll(out, "{{title}}", title);
    out             = replaceAll(out, "{{nav}}", nav);
    out             = replaceAll(out, "{{content}}", content);
    out             = replaceAll(out, "{{base_url}}", baseUrl);
    return out;
  }

  std::string renderMarkdown(const std::string &markdown) {
    std::string out;
    unsigned    parserFlags = MD_DIALECT_GITHUB;
    md_html(markdown.data(), static_cast<MD_SIZE>(markdown.size()), mdSink,
            &out, parserFlags, 0);
    return addHeadingAnchors(out);
  }

}  // namespace docsgen
