// Copyright (c) 2014-2016 Thomas Fussell
// Copyright (c) 2010-2015 openpyxl
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, WRISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE
//
// @license: http://www.opensource.org/licenses/mit-license.php
// @author: see AUTHORS file

#include <algorithm>
#include <cctype> // for std::tolower
#include <unordered_set>
#include <utility> // for std::move

#include <xlnt/serialization/style_serializer.hpp>
#include <detail/workbook_impl.hpp>
#include <detail/stylesheet.hpp>
#include <xlnt/cell/cell.hpp>
#include <xlnt/serialization/xml_document.hpp>
#include <xlnt/serialization/xml_node.hpp>
#include <xlnt/styles/alignment.hpp>
#include <xlnt/styles/border.hpp>
#include <xlnt/styles/fill.hpp>
#include <xlnt/styles/font.hpp>
#include <xlnt/styles/format.hpp>
#include <xlnt/styles/style.hpp>
#include <xlnt/styles/number_format.hpp>
#include <xlnt/styles/protection.hpp>
#include <xlnt/workbook/worksheet_iterator.hpp>
#include <xlnt/worksheet/cell_iterator.hpp>
#include <xlnt/worksheet/cell_vector.hpp>
#include <xlnt/worksheet/range.hpp>
#include <xlnt/worksheet/worksheet.hpp>
#include <xlnt/worksheet/range_iterator.hpp>

namespace {

// Miscellaneous Functions

bool equals_case_insensitive(const std::string &left, const std::string &right)
{
    if (left.size() != right.size())
    {
        return false;
    }

    for (std::size_t i = 0; i < left.size(); i++)
    {
        if (std::tolower(left[i]) != std::tolower(right[i]))
        {
            return false;
        }
    }
    
    return true;
}

bool is_true(const std::string &bool_string)
{
    return bool_string == "1" || bool_string == "true";
}

std::size_t string_to_size_t(const std::string &s)
{
#if ULLONG_MAX == SIZE_MAX
	return std::stoull(s);
#else
	return std::stoul(s);
#endif
}

// Enumerations from string

xlnt::protection::type protection_type_from_string(const std::string &type_string)
{
    if (equals_case_insensitive(type_string, "true"))
    {
        return xlnt::protection::type::protected_;
    }
    else if (equals_case_insensitive(type_string, "inherit"))
    {
        return xlnt::protection::type::inherit;
    }

    return xlnt::protection::type::unprotected;
};

xlnt::font::underline_style underline_style_from_string(const std::string &underline_string)
{
    if (equals_case_insensitive(underline_string, "none"))
    {
        return xlnt::font::underline_style::none;
    }
    else if (equals_case_insensitive(underline_string, "single"))
    {
        return xlnt::font::underline_style::single;
    }
    else if (equals_case_insensitive(underline_string, "single-accounting"))
    {
        return xlnt::font::underline_style::single_accounting;
    }
    else if (equals_case_insensitive(underline_string, "double"))
    {
        return xlnt::font::underline_style::double_;
    }
    else if (equals_case_insensitive(underline_string, "double-accounting"))
    {
        return xlnt::font::underline_style::double_accounting;
    }

    return xlnt::font::underline_style::none;
}

xlnt::fill::pattern_type pattern_fill_type_from_string(const std::string &fill_type)
{
    if (equals_case_insensitive(fill_type, "none") || fill_type.empty())
    {
        return xlnt::fill::pattern_type::none;
    }
    else if (equals_case_insensitive(fill_type, "solid"))
    {
        return xlnt::fill::pattern_type::solid;
    }
    else if (equals_case_insensitive(fill_type, "gray125"))
    {
        return xlnt::fill::pattern_type::gray125;
    }

    std::string message = "unknown fill type: ";
    message.append(fill_type);
    throw std::runtime_error(message);
};

xlnt::border_style border_style_from_string(const std::string &border_style_string)
{
    if (equals_case_insensitive(border_style_string, "none"))
    {
        return xlnt::border_style::none;
    }
    else if (equals_case_insensitive(border_style_string, "dashdot"))
    {
        return xlnt::border_style::dashdot;
    }
    else if (equals_case_insensitive(border_style_string, "dashdotdot"))
    {
        return xlnt::border_style::dashdotdot;
    }
    else if (equals_case_insensitive(border_style_string, "dashed"))
    {
        return xlnt::border_style::dashed;
    }
    else if (equals_case_insensitive(border_style_string, "dotted"))
    {
        return xlnt::border_style::dotted;
    }
    else if (equals_case_insensitive(border_style_string, "double"))
    {
        return xlnt::border_style::double_;
    }
    else if (equals_case_insensitive(border_style_string, "hair"))
    {
        return xlnt::border_style::hair;
    }
    else if (equals_case_insensitive(border_style_string, "medium"))
    {
        return xlnt::border_style::medium;
    }
    else if (equals_case_insensitive(border_style_string, "mediumdashdot"))
    {
        return xlnt::border_style::mediumdashdot;
    }
    else if (equals_case_insensitive(border_style_string, "mediumdashdotdot"))
    {
        return xlnt::border_style::mediumdashdotdot;
    }
    else if (equals_case_insensitive(border_style_string, "mediumdashed"))
    {
        return xlnt::border_style::mediumdashed;
    }
    else if (equals_case_insensitive(border_style_string, "slantdashdot"))
    {
        return xlnt::border_style::slantdashdot;
    }
    else if (equals_case_insensitive(border_style_string, "thick"))
    {
        return xlnt::border_style::thick;
    }
    else if (equals_case_insensitive(border_style_string, "thin"))
    {
        return xlnt::border_style::thin;
    }

    std::string message = "unknown border style: ";
    message.append(border_style_string);
    throw std::runtime_error(message);
}

xlnt::vertical_alignment vertical_alignment_from_string(const std::string &vertical_alignment_string)
{
    if (vertical_alignment_string == "bottom")
    {
        return xlnt::vertical_alignment::bottom;
    }
    else if (vertical_alignment_string == "center")
    {
        return xlnt::vertical_alignment::center;
    }
    else if (vertical_alignment_string == "justify")
    {
        return xlnt::vertical_alignment::justify;
    }
    else if (vertical_alignment_string == "top")
    {
        return xlnt::vertical_alignment::top;
    }

    std::string message = "unknown vertical alignment: ";
    message.append(vertical_alignment_string);
    throw std::runtime_error(message);
}

xlnt::horizontal_alignment horizontal_alignment_from_string(const std::string &horizontal_alignment_string)
{
    if (horizontal_alignment_string == "left")
    {
        return xlnt::horizontal_alignment::left;
    }
    else if (horizontal_alignment_string == "center")
    {
        return xlnt::horizontal_alignment::center;
    }
    else if (horizontal_alignment_string == "center-continuous")
    {
        return xlnt::horizontal_alignment::center_continuous;
    }
    else if (horizontal_alignment_string == "right")
    {
        return xlnt::horizontal_alignment::right;
    }
    else if (horizontal_alignment_string == "justify")
    {
        return xlnt::horizontal_alignment::justify;
    }
    else if (horizontal_alignment_string == "general")
    {
        return xlnt::horizontal_alignment::general;
    }

    std::string message = "unknown horizontal alignment: ";
    message.append(horizontal_alignment_string);
    throw std::runtime_error(message);
}

// Reading

xlnt::protection read_protection(const xlnt::xml_node &protection_node)
{
    xlnt::protection prot;

    prot.set_locked(protection_type_from_string(protection_node.get_attribute("locked")));
    prot.set_hidden(protection_type_from_string(protection_node.get_attribute("hidden")));

    return std::move(prot);
}

xlnt::alignment read_alignment(const xlnt::xml_node &alignment_node)
{
    xlnt::alignment align;

    align.set_wrap_text(is_true(alignment_node.get_attribute("wrapText")));
    align.set_shrink_to_fit(is_true(alignment_node.get_attribute("shrinkToFit")));

    bool has_vertical = alignment_node.has_attribute("vertical");

    if (has_vertical)
    {
        std::string vertical = alignment_node.get_attribute("vertical");
        align.set_vertical(vertical_alignment_from_string(vertical));
    }

    bool has_horizontal = alignment_node.has_attribute("horizontal");

    if (has_horizontal)
    {
        std::string horizontal = alignment_node.get_attribute("horizontal");
        align.set_horizontal(horizontal_alignment_from_string(horizontal));
    }

    return align;
}

void read_number_formats(const xlnt::xml_node &number_formats_node, std::vector<xlnt::number_format> &number_formats)
{
    number_formats.clear();

    for (auto num_fmt_node : number_formats_node.get_children())
    {
        if (num_fmt_node.get_name() != "numFmt")
        {
            continue;
        }

        auto format_string = num_fmt_node.get_attribute("formatCode");

        if (format_string == "GENERAL")
        {
            format_string = "General";
        }

        xlnt::number_format nf;

        nf.set_format_string(format_string);
        nf.set_id(string_to_size_t(num_fmt_node.get_attribute("numFmtId")));

        number_formats.push_back(nf);
    }
}

xlnt::color read_color(const xlnt::xml_node &color_node)
{
    if (color_node.has_attribute("rgb"))
    {
        return xlnt::color(xlnt::color::type::rgb, color_node.get_attribute("rgb"));
    }
    else if (color_node.has_attribute("theme"))
    {
        return xlnt::color(xlnt::color::type::theme, string_to_size_t(color_node.get_attribute("theme")));
    }
    else if (color_node.has_attribute("indexed"))
    {
        return xlnt::color(xlnt::color::type::indexed, string_to_size_t(color_node.get_attribute("indexed")));
    }
    else if (color_node.has_attribute("auto"))
    {
        return xlnt::color(xlnt::color::type::auto_, string_to_size_t(color_node.get_attribute("auto")));
    }

    throw std::runtime_error("bad color");
}

xlnt::font read_font(const xlnt::xml_node &font_node)
{
    xlnt::font new_font;

    new_font.set_size(string_to_size_t(font_node.get_child("sz").get_attribute("val")));
    new_font.set_name(font_node.get_child("name").get_attribute("val"));

    if (font_node.has_child("color"))
    {
        new_font.set_color(read_color(font_node.get_child("color")));
    }

    if (font_node.has_child("family"))
    {
        new_font.set_family(string_to_size_t(font_node.get_child("family").get_attribute("val")));
    }

    if (font_node.has_child("scheme"))
    {
        new_font.set_scheme(font_node.get_child("scheme").get_attribute("val"));
    }

    if (font_node.has_child("b"))
    {
        if(font_node.get_child("b").has_attribute("val"))
        {
            new_font.set_bold(is_true(font_node.get_child("b").get_attribute("val")));
        }
        else
        {
            new_font.set_bold(true);
        }
    }

    if (font_node.has_child("strike"))
    {
        if(font_node.get_child("strike").has_attribute("val"))
        {
            new_font.set_strikethrough(is_true(font_node.get_child("strike").get_attribute("val")));
        }
        else
        {
            new_font.set_strikethrough(true);
        }
    }

    if (font_node.has_child("i"))
    {
        if(font_node.get_child("i").has_attribute("val"))
        {
            new_font.set_italic(is_true(font_node.get_child("i").get_attribute("val")));
        }
        else
        {
            new_font.set_italic(true);
        }
    }

    if (font_node.has_child("u"))
    {
        if (font_node.get_child("u").has_attribute("val"))
        {
            std::string underline_string = font_node.get_child("u").get_attribute("val");
            new_font.set_underline(underline_style_from_string(underline_string));
        }
        else
        {
            new_font.set_underline(xlnt::font::underline_style::single);
        }
    }

    return new_font;
}

void read_fonts(const xlnt::xml_node &fonts_node, std::vector<xlnt::font> &fonts)
{
    fonts.clear();

    for (auto font_node : fonts_node.get_children())
    {
        fonts.push_back(read_font(font_node));
    }
}

void read_indexed_colors(const xlnt::xml_node &indexed_colors_node, std::vector<xlnt::color> &colors)
{
    for (auto color_node : indexed_colors_node.get_children())
    {
        colors.push_back(read_color(color_node));
    }
}

void read_colors(const xlnt::xml_node &colors_node, std::vector<xlnt::color> &colors)
{
    colors.clear();

    if (colors_node.has_child("indexedColors"))
    {
        read_indexed_colors(colors_node.get_child("indexedColors"), colors);
    }
}

xlnt::fill read_fill(const xlnt::xml_node &fill_node)
{
    xlnt::fill new_fill;

    if (fill_node.has_child("patternFill"))
    {
        new_fill.set_type(xlnt::fill::type::pattern);

        auto pattern_fill_node = fill_node.get_child("patternFill");
        new_fill.set_pattern_type(pattern_fill_type_from_string(pattern_fill_node.get_attribute("patternType")));

        if (pattern_fill_node.has_child("bgColor"))
        {
            new_fill.get_background_color() = read_color(pattern_fill_node.get_child("bgColor"));
        }

        if (pattern_fill_node.has_child("fgColor"))
        {
            new_fill.get_foreground_color() = read_color(pattern_fill_node.get_child("fgColor"));
        }
    }

    return new_fill;
}

void read_fills(const xlnt::xml_node &fills_node, std::vector<xlnt::fill> &fills)
{
    fills.clear();

    for (auto fill_node : fills_node.get_children())
    {
        fills.emplace_back();
        fills.back() = read_fill(fill_node);
    }
}

xlnt::side read_side(const xlnt::xml_node &side_node)
{
    xlnt::side new_side;

    if (side_node.has_attribute("style"))
    {
        new_side.get_border_style() = border_style_from_string(side_node.get_attribute("style"));
    }

    if (side_node.has_child("color"))
    {
        new_side.get_color() = read_color(side_node.get_child("color"));
    }

    return new_side;
}

xlnt::border read_border(const xlnt::xml_node &border_node)
{
    xlnt::border new_border;

    if (border_node.has_child("start"))
    {
        new_border.get_start() = read_side(border_node.get_child("start"));
    }

    if (border_node.has_child("end"))
    {
        new_border.get_end() = read_side(border_node.get_child("end"));
    }

    if (border_node.has_child("left"))
    {
        new_border.get_left() = read_side(border_node.get_child("left"));
    }

    if (border_node.has_child("right"))
    {
        new_border.get_right() = read_side(border_node.get_child("right"));
    }

    if (border_node.has_child("top"))
    {
        new_border.get_top() = read_side(border_node.get_child("top"));
    }

    if (border_node.has_child("bottom"))
    {
        new_border.get_bottom() = read_side(border_node.get_child("bottom"));
    }

    if (border_node.has_child("diagonal"))
    {
        new_border.get_diagonal() = read_side(border_node.get_child("diagonal"));
    }

    if (border_node.has_child("vertical"))
    {
        new_border.get_vertical() = read_side(border_node.get_child("vertical"));
    }

    if (border_node.has_child("horizontal"))
    {
        new_border.get_horizontal() = read_side(border_node.get_child("horizontal"));
    }

    return new_border;
}

void read_borders(const xlnt::xml_node &borders_node, std::vector<xlnt::border> &borders)
{
    borders.clear();

    for (auto border_node : borders_node.get_children())
    {
        borders.push_back(read_border(border_node));
    }
}


bool read_base_format(const xlnt::xml_node &format_node, const xlnt::detail::stylesheet &stylesheet, xlnt::base_format &f)
{
    // Alignment
    f.alignment_applied(format_node.has_child("alignment") || is_true(format_node.get_attribute("applyAlignment")));

    if (f.alignment_applied())
    {
        auto inline_alignment = read_alignment(format_node.get_child("alignment"));
        f.set_alignment(inline_alignment);
    }
    
    // Border
    auto border_index = format_node.has_attribute("borderId") ? string_to_size_t(format_node.get_attribute("borderId")) : 0;
    f.set_border(stylesheet.borders.at(border_index));
    f.border_applied(is_true(format_node.get_attribute("applyBorder")));
    
    // Fill
    auto fill_index = format_node.has_attribute("fillId") ? string_to_size_t(format_node.get_attribute("fillId")) : 0;
    f.set_fill(stylesheet.fills.at(fill_index));
    f.fill_applied(is_true(format_node.get_attribute("applyFill")));
    
    // Font
    auto font_index = format_node.has_attribute("fontId") ? string_to_size_t(format_node.get_attribute("fontId")) : 0;
    f.set_font(stylesheet.fonts.at(font_index));
    f.font_applied(is_true(format_node.get_attribute("applyFont")));

    // Number Format
    auto number_format_id = string_to_size_t(format_node.get_attribute("numFmtId"));

    bool builtin_format = true;

    for (const auto &num_fmt : stylesheet.number_formats)
    {
        if (static_cast<std::size_t>(num_fmt.get_id()) == number_format_id)
        {
            f.set_number_format(num_fmt);
            builtin_format = false;
            break;
        }
    }

    if (builtin_format)
    {
        f.set_number_format(xlnt::number_format::from_builtin_id(number_format_id));
    }
    
    f.number_format_applied(is_true(format_node.get_attribute("applyNumberFormat")));

    // Protection
    f.protection_applied(format_node.has_attribute("protection") || is_true(format_node.get_attribute("applyProtection")));

    if (f.protection_applied())
    {
        auto inline_protection = read_protection(format_node.get_child("protection"));
        f.set_protection(inline_protection);
    }
    
    return true;
}


void read_formats(const xlnt::xml_node &formats_node, const xlnt::detail::stylesheet &stylesheet,
    std::vector<xlnt::format> &formats, std::vector<std::string> &format_styles)
{
    for (auto format_node : formats_node.get_children())
    {
        if (format_node.get_name() != "xf")
        {
            continue;
        }

        xlnt::format format;
        read_base_format(format_node, stylesheet, format);
        
        // TODO do all formats have xfId?
        if(format_node.has_attribute("xfId"))
        {
            auto style_index = string_to_size_t(format_node.get_attribute("xfId"));
            auto style_name = stylesheet.style_name_map.at(style_index);
            format_styles.push_back(style_name);
        }
        else
        {
            format_styles.push_back("");
        }
        
        formats.push_back(format);
    }
}

xlnt::style read_style(const xlnt::xml_node &style_node, const xlnt::xml_node &style_format_node, const xlnt::detail::stylesheet &stylesheet)
{
    xlnt::style s;

    read_base_format(style_format_node, stylesheet, s);

    s.set_name(style_node.get_attribute("name"));
    s.set_hidden(style_node.has_attribute("hidden") && is_true(style_node.get_attribute("hidden")));
    s.set_builtin_id(string_to_size_t(style_node.get_attribute("builtinId")));
    
    return s;
}

void read_styles(const xlnt::xml_node &styles_node, const xlnt::xml_node &style_formats_node, const xlnt::detail::stylesheet stylesheet, std::vector<xlnt::style> &styles, std::unordered_map<std::size_t, std::string> &style_names)
{
    std::size_t style_index = 0;
    
    for (auto cell_style_format_node : style_formats_node.get_children())
    {
        bool match = false;
        
        for (auto cell_style_node : styles_node.get_children())
        {
            auto cell_style_format_index = std::stoull(cell_style_node.get_attribute("xfId"));
            
            if (cell_style_format_index == style_index)
            {
                styles.push_back(read_style(cell_style_node, cell_style_format_node, stylesheet));
                style_names[style_index] = styles.back().get_name();
                match = true;
                
                break;
            }
        }

        style_index++;
    }
}

bool write_fonts(const std::vector<xlnt::font> &fonts, xlnt::xml_node &fonts_node)
{
    fonts_node.add_attribute("count", std::to_string(fonts.size()));
    // TODO: what does this do?
    // fonts_node.add_attribute("x14ac:knownFonts", "1");

    for (auto &f : fonts)
    {
        auto font_node = fonts_node.add_child("font");

        if (f.is_bold())
        {
            auto bold_node = font_node.add_child("b");
            bold_node.add_attribute("val", "1");
        }

        if (f.is_italic())
        {
            auto italic_node = font_node.add_child("i");
            italic_node.add_attribute("val", "1");
        }

        if (f.is_underline())
        {
            auto underline_node = font_node.add_child("u");

            switch (f.get_underline())
            {
            case xlnt::font::underline_style::single:
                underline_node.add_attribute("val", "single");
                break;
            default:
                break;
            }
        }

        if (f.is_strikethrough())
        {
            auto strike_node = font_node.add_child("strike");
            strike_node.add_attribute("val", "1");
        }

        auto size_node = font_node.add_child("sz");
        size_node.add_attribute("val", std::to_string(f.get_size()));

        auto color_node = font_node.add_child("color");

        if (f.get_color().get_type() == xlnt::color::type::indexed)
        {
            color_node.add_attribute("indexed", std::to_string(f.get_color().get_index()));
        }
        else if (f.get_color().get_type() == xlnt::color::type::theme)
        {
            color_node.add_attribute("theme", std::to_string(f.get_color().get_theme()));
        }
        else if (f.get_color().get_type() == xlnt::color::type::auto_)
        {
            color_node.add_attribute("auto", std::to_string(f.get_color().get_auto()));
        }
        else if (f.get_color().get_type() == xlnt::color::type::rgb)
        {
            color_node.add_attribute("rgb", f.get_color().get_rgb_string());
        }
        
        auto name_node = font_node.add_child("name");
        name_node.add_attribute("val", f.get_name());

        if (f.has_family())
        {
            auto family_node = font_node.add_child("family");
            family_node.add_attribute("val", std::to_string(f.get_family()));
        }

        if (f.has_scheme())
        {
            auto scheme_node = font_node.add_child("scheme");
            scheme_node.add_attribute("val", "minor");
        }
    }
    
    return true;
}

bool write_fills(const std::vector<xlnt::fill> &fills, xlnt::xml_node &fills_node)
{
    fills_node.add_attribute("count", std::to_string(fills.size()));

    for (auto &fill_ : fills)
    {
        auto fill_node = fills_node.add_child("fill");

        if (fill_.get_type() == xlnt::fill::type::pattern)
        {
            auto pattern_fill_node = fill_node.add_child("patternFill");
            auto type_string = fill_.get_pattern_type_string();
            pattern_fill_node.add_attribute("patternType", type_string);
            
            if (fill_.get_pattern_type() != xlnt::fill::pattern_type::solid) continue;

            if (fill_.get_foreground_color())
            {
                auto fg_color_node = pattern_fill_node.add_child("fgColor");

                switch (fill_.get_foreground_color()->get_type())
                {
                case xlnt::color::type::auto_:
                    fg_color_node.add_attribute("auto", std::to_string(fill_.get_foreground_color()->get_auto()));
                    break;
                case xlnt::color::type::theme:
                    fg_color_node.add_attribute("theme", std::to_string(fill_.get_foreground_color()->get_theme()));
                    break;
                case xlnt::color::type::indexed:
                    fg_color_node.add_attribute("indexed", std::to_string(fill_.get_foreground_color()->get_index()));
                    break;
                case xlnt::color::type::rgb:
                    fg_color_node.add_attribute("rgb", fill_.get_foreground_color()->get_rgb_string());
                    break;
                default:
                    throw std::runtime_error("bad type");
                }
            }

            if (fill_.get_background_color())
            {
                auto bg_color_node = pattern_fill_node.add_child("bgColor");

                switch (fill_.get_background_color()->get_type())
                {
                case xlnt::color::type::auto_:
                    bg_color_node.add_attribute("auto", std::to_string(fill_.get_background_color()->get_auto()));
                    break;
                case xlnt::color::type::theme:
                    bg_color_node.add_attribute("theme", std::to_string(fill_.get_background_color()->get_theme()));
                    break;
                case xlnt::color::type::indexed:
                    bg_color_node.add_attribute("indexed", std::to_string(fill_.get_background_color()->get_index()));
                    break;
                case xlnt::color::type::rgb:
                    bg_color_node.add_attribute("rgb", fill_.get_background_color()->get_rgb_string());
                    break;
                default:
                    throw std::runtime_error("bad type");
                }
            }
        }
        else if (fill_.get_type() == xlnt::fill::type::solid)
        {
            auto solid_fill_node = fill_node.add_child("solidFill");
            solid_fill_node.add_child("color");
        }
        else if (fill_.get_type() == xlnt::fill::type::gradient)
        {
            auto gradient_fill_node = fill_node.add_child("gradientFill");

            if (fill_.get_gradient_type_string() == "linear")
            {
                gradient_fill_node.add_attribute("degree", std::to_string(fill_.get_rotation()));
            }
            else if (fill_.get_gradient_type_string() == "path")
            {
                gradient_fill_node.add_attribute("left", std::to_string(fill_.get_gradient_left()));
                gradient_fill_node.add_attribute("right", std::to_string(fill_.get_gradient_right()));
                gradient_fill_node.add_attribute("top", std::to_string(fill_.get_gradient_top()));
                gradient_fill_node.add_attribute("bottom", std::to_string(fill_.get_gradient_bottom()));

                auto start_node = gradient_fill_node.add_child("stop");
                start_node.add_attribute("position", "0");

                auto end_node = gradient_fill_node.add_child("stop");
                end_node.add_attribute("position", "1");
            }
        }
    }

    return true;
}

bool write_borders(const std::vector<xlnt::border> &borders, xlnt::xml_node &borders_node)
{
    borders_node.add_attribute("count", std::to_string(borders.size()));

    for (const auto &border_ : borders)
    {
        auto border_node = borders_node.add_child("border");

        std::vector<std::tuple<std::string, const std::experimental::optional<xlnt::side>>> sides;
        
        sides.push_back(std::make_tuple("start", border_.get_start()));
        sides.push_back(std::make_tuple("end", border_.get_end()));
        sides.push_back(std::make_tuple("left", border_.get_left()));
        sides.push_back(std::make_tuple("right", border_.get_right()));
        sides.push_back(std::make_tuple("top", border_.get_top()));
        sides.push_back(std::make_tuple("bottom", border_.get_bottom()));
        sides.push_back(std::make_tuple("diagonal", border_.get_diagonal()));
        sides.push_back(std::make_tuple("vertical", border_.get_vertical()));
        sides.push_back(std::make_tuple("horizontal", border_.get_horizontal()));

        for (const auto &side_tuple : sides)
        {
            std::string current_name = std::get<0>(side_tuple);
            const auto current_side = std::get<1>(side_tuple);

            if (current_side)
            {
                auto side_node = border_node.add_child(current_name);

                if (current_side->get_border_style())
                {
                    std::string style_string;

                    switch (*current_side->get_border_style())
                    {
                    case xlnt::border_style::none:
                        style_string = "none";
                        break;
                    case xlnt::border_style::dashdot:
                        style_string = "dashDot";
                        break;
                    case xlnt::border_style::dashdotdot:
                        style_string = "dashDotDot";
                        break;
                    case xlnt::border_style::dashed:
                        style_string = "dashed";
                        break;
                    case xlnt::border_style::dotted:
                        style_string = "dotted";
                        break;
                    case xlnt::border_style::double_:
                        style_string = "double";
                        break;
                    case xlnt::border_style::hair:
                        style_string = "hair";
                        break;
                    case xlnt::border_style::medium:
                        style_string = "thin";
                        break;
                    case xlnt::border_style::mediumdashdot:
                        style_string = "mediumDashDot";
                        break;
                    case xlnt::border_style::mediumdashdotdot:
                        style_string = "mediumDashDotDot";
                        break;
                    case xlnt::border_style::mediumdashed:
                        style_string = "mediumDashed";
                        break;
                    case xlnt::border_style::slantdashdot:
                        style_string = "slantDashDot";
                        break;
                    case xlnt::border_style::thick:
                        style_string = "thick";
                        break;
                    case xlnt::border_style::thin:
                        style_string = "thin";
                        break;
                    default:
                        throw std::runtime_error("invalid style");
                    }

                    side_node.add_attribute("style", style_string);
                }

                if (current_side->get_color())
                {
                    auto color_node = side_node.add_child("color");

                    if (current_side->get_color()->get_type() == xlnt::color::type::indexed)
                    {
                        color_node.add_attribute("indexed", std::to_string(current_side->get_color()->get_index()));
                    }
                    else if (current_side->get_color()->get_type() == xlnt::color::type::theme)
                    {
                        color_node.add_attribute("theme", std::to_string(current_side->get_color()->get_theme()));
                    }
                    else if (current_side->get_color()->get_type() == xlnt::color::type::rgb)
                    {
                        color_node.add_attribute("rgb", current_side->get_color()->get_rgb_string());
                    }
                    else
                    {
                        throw std::runtime_error("invalid color type");
                    }
                }
            }
        }
    }

    return true;
}

bool write_base_format(const xlnt::base_format &xf, const xlnt::detail::stylesheet &stylesheet, xlnt::xml_node xf_node)
{
    xf_node.add_attribute("numFmtId", std::to_string(xf.get_number_format().get_id()));
     
    auto font_id = std::distance(stylesheet.fonts.begin(), std::find(stylesheet.fonts.begin(), stylesheet.fonts.end(), xf.get_font()));
    xf_node.add_attribute("fontId", std::to_string(font_id));

    auto fill_id = std::distance(stylesheet.fills.begin(), std::find(stylesheet.fills.begin(), stylesheet.fills.end(), xf.get_fill()));
    xf_node.add_attribute("fillId", std::to_string(fill_id));

    auto border_id = std::distance(stylesheet.borders.begin(), std::find(stylesheet.borders.begin(), stylesheet.borders.end(), xf.get_border()));
    xf_node.add_attribute("borderId", std::to_string(border_id));

    if(xf.number_format_applied()) xf_node.add_attribute("applyNumberFormat", "1");
    if(xf.fill_applied()) xf_node.add_attribute("applyFill", "1");
    if(xf.font_applied()) xf_node.add_attribute("applyFont", "1");
    if(xf.border_applied()) xf_node.add_attribute("applyBorder", "1");
    if(xf.alignment_applied()) xf_node.add_attribute("applyAlignment", "1");
    if(xf.protection_applied()) xf_node.add_attribute("applyProtection", "1");

    if (xf.alignment_applied())
    {
        auto alignment_node = xf_node.add_child("alignment");

        if (xf.get_alignment().has_vertical())
        {
            switch (xf.get_alignment().get_vertical())
            {
            case xlnt::vertical_alignment::bottom:
                alignment_node.add_attribute("vertical", "bottom");
                break;
            case xlnt::vertical_alignment::center:
                alignment_node.add_attribute("vertical", "center");
                break;
            case xlnt::vertical_alignment::justify:
                alignment_node.add_attribute("vertical", "justify");
                break;
            case xlnt::vertical_alignment::top:
                alignment_node.add_attribute("vertical", "top");
                break;
            default:
                throw std::runtime_error("invalid alignment");
            }
        }

        if (xf.get_alignment().has_horizontal())
        {
            switch (xf.get_alignment().get_horizontal())
            {
            case xlnt::horizontal_alignment::center:
                alignment_node.add_attribute("horizontal", "center");
                break;
            case xlnt::horizontal_alignment::center_continuous:
                alignment_node.add_attribute("horizontal", "center_continuous");
                break;
            case xlnt::horizontal_alignment::general:
                alignment_node.add_attribute("horizontal", "general");
                break;
            case xlnt::horizontal_alignment::justify:
                alignment_node.add_attribute("horizontal", "justify");
                break;
            case xlnt::horizontal_alignment::left:
                alignment_node.add_attribute("horizontal", "left");
                break;
            case xlnt::horizontal_alignment::right:
                alignment_node.add_attribute("horizontal", "right");
                break;
            default:
                throw std::runtime_error("invalid alignment");
            }
        }

        if (xf.get_alignment().get_wrap_text())
        {
            alignment_node.add_attribute("wrapText", "1");
        }
        
        if (xf.get_alignment().get_shrink_to_fit())
        {
            alignment_node.add_attribute("shrinkToFit", "1");
        }
    }
    
    //TODO protection!
    
    return true;
}

bool write_styles(const xlnt::detail::stylesheet &stylesheet, xlnt::xml_node &styles_node, xlnt::xml_node &style_formats_node)
{
    style_formats_node.add_attribute("count", std::to_string(stylesheet.styles.size()));
    styles_node.add_attribute("count", std::to_string(stylesheet.styles.size()));
    std::size_t style_index = 0;

    for(auto &current_style : stylesheet.styles)
    {
        auto xf_node = style_formats_node.add_child("xf");
        write_base_format(current_style, stylesheet, xf_node);

        auto cell_style_node = styles_node.add_child("cellStyle");
        
        cell_style_node.add_attribute("name", current_style.get_name());
        cell_style_node.add_attribute("xfId", std::to_string(style_index++));
        cell_style_node.add_attribute("builtinId", std::to_string(current_style.get_builtin_id()));
        
        if (current_style.get_hidden())
        {
            cell_style_node.add_attribute("hidden", "1");
        }
    }

    return true;
}

bool write_formats(const xlnt::detail::stylesheet &stylesheet, xlnt::xml_node &formats_node)
{
    formats_node.add_attribute("count", std::to_string(stylesheet.formats.size()));

    auto format_style_iterator = stylesheet.format_styles.begin();

    for(auto &current_format : stylesheet.formats)
    {
        auto xf_node = formats_node.add_child("xf");
        write_base_format(current_format, stylesheet, xf_node);
        
        const auto format_style_name = *(format_style_iterator++);
        
        if(!format_style_name.empty())
        {
            auto style = std::find_if(stylesheet.styles.begin(), stylesheet.styles.end(),
                [&](const xlnt::style &s) { return s.get_name() == format_style_name; });
            auto style_index = std::distance(stylesheet.styles.begin(), style);
            
            xf_node.add_attribute("xfId", std::to_string(style_index));
        }
    }

    return true;
}

bool write_dxfs(xlnt::xml_node &dxfs_node)
{
    dxfs_node.add_attribute("count", "0");
    return true;
}

bool write_table_styles(xlnt::xml_node &table_styles_node)
{
    table_styles_node.add_attribute("count", "0");
    table_styles_node.add_attribute("defaultTableStyle", "TableStyleMedium9");
    table_styles_node.add_attribute("defaultPivotStyle", "PivotStyleMedium7");
    
    return true;
}

bool write_colors(const std::vector<xlnt::color> &colors, xlnt::xml_node &colors_node)
{
    auto indexed_colors_node = colors_node.add_child("indexedColors");

    for (auto &c : colors)
    {
        indexed_colors_node.add_child("rgbColor").add_attribute("rgb", c.get_rgb_string());
    }
    
    return true;
}

bool write_ext_list(xlnt::xml_node &ext_list_node)
{
    auto ext_node = ext_list_node.add_child("ext");
    ext_node.add_attribute("uri", "{EB79DEF2-80B8-43e5-95BD-54CBDDF9020C}");
    ext_node.add_attribute("xmlns:x14", "http://schemas.microsoft.com/office/spreadsheetml/2009/9/main");
    ext_node.add_child("x14:slicerStyles").add_attribute("defaultSlicerStyle", "SlicerStyleLight1");
    
    return true;
}

bool write_number_formats(const std::vector<xlnt::number_format> &number_formats, xlnt::xml_node &number_formats_node)
{
    number_formats_node.add_attribute("count", std::to_string(number_formats.size()));

    for (const auto &num_fmt : number_formats)
    {
        auto num_fmt_node = number_formats_node.add_child("numFmt");
        num_fmt_node.add_attribute("numFmtId", std::to_string(num_fmt.get_id()));
        num_fmt_node.add_attribute("formatCode", num_fmt.get_format_string());
    }

    return true;
}

} // namespace

namespace xlnt {

style_serializer::style_serializer(detail::stylesheet &stylesheet) : stylesheet_(stylesheet)
{
}

bool style_serializer::read_stylesheet(const xml_document &xml)
{
    auto stylesheet_node = xml.get_child("styleSheet");

    read_borders(stylesheet_node.get_child("borders"), stylesheet_.borders);
    read_fills(stylesheet_node.get_child("fills"), stylesheet_.fills);
    read_fonts(stylesheet_node.get_child("fonts"), stylesheet_.fonts);
    read_number_formats(stylesheet_node.get_child("numFmts"), stylesheet_.number_formats);
    read_colors(stylesheet_node.get_child("colors"), stylesheet_.colors);
    read_styles(stylesheet_node.get_child("cellStyles"), stylesheet_node.get_child("cellStyleXfs"), stylesheet_, stylesheet_.styles, stylesheet_.style_name_map);
    read_formats(stylesheet_node.get_child("cellXfs"), stylesheet_, stylesheet_.formats, stylesheet_.format_styles);

    return true;
}

bool style_serializer::write_stylesheet(xml_document &doc)
{
    auto root_node = doc.add_child("styleSheet");
    doc.add_namespace("", "http://schemas.openxmlformats.org/spreadsheetml/2006/main");
    doc.add_namespace("mc", "http://schemas.openxmlformats.org/markup-compatibility/2006");
    root_node.add_attribute("mc:Ignorable", "x14ac");
    doc.add_namespace("x14ac", "http://schemas.microsoft.com/office/spreadsheetml/2009/9/ac");

    if (!stylesheet_.number_formats.empty())
    {
        auto number_formats_node = root_node.add_child("numFmts");
        write_number_formats(stylesheet_.number_formats, number_formats_node);
    }

    if (!stylesheet_.fonts.empty())
    {
        auto fonts_node = root_node.add_child("fonts");
        write_fonts(stylesheet_.fonts, fonts_node);
    }

    if (!stylesheet_.fills.empty())
    {
        auto fills_node = root_node.add_child("fills");
        write_fills(stylesheet_.fills, fills_node);
    }

    if (!stylesheet_.borders.empty())
    {
        auto borders_node = root_node.add_child("borders");
        write_borders(stylesheet_.borders, borders_node);
    }
    
    auto cell_style_xfs_node = root_node.add_child("cellStyleXfs");
    
    auto cell_xfs_node = root_node.add_child("cellXfs");
    write_formats(stylesheet_, cell_xfs_node);

    auto cell_styles_node = root_node.add_child("cellStyles");
    write_styles(stylesheet_, cell_styles_node, cell_style_xfs_node);
    
    auto dxfs_node = root_node.add_child("dxfs");
    write_dxfs(dxfs_node);

    auto table_styles_node = root_node.add_child("tableStyles");
    write_table_styles(table_styles_node);

    if(!stylesheet_.colors.empty())
    {
        auto colors_node = root_node.add_child("colors");
        write_colors(stylesheet_.colors, colors_node);
    }
    
    auto ext_list_node = root_node.add_child("extLst");
    write_ext_list(ext_list_node);

    return true;
}

} // namespace xlnt
