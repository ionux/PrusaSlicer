///|/ Copyright (c) Prusa Research 2020 - 2023 Oleksandra Iushchenko @YuSanka, Vojtěch Bubník @bubnikv, Tomáš Mészáros @tamasmeszaros, Lukáš Matěna @lukasmatena, Enrico Turri @enricoturri1966
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/

#include "ImGuiDoubleSlider.hpp"

namespace DoubleSlider {

const ImU32 tooltip_bg_clr  = ImGui::ColorConvertFloat4ToU32(ImGuiPureWrap::COL_GREY_LIGHT);
const ImU32 thumb_bg_clr    = ImGui::ColorConvertFloat4ToU32(ImGuiPureWrap::COL_ORANGE_LIGHT);
const ImU32 groove_bg_clr   = ImGui::ColorConvertFloat4ToU32(ImGuiPureWrap::COL_WINDOW_BACKGROUND);
const ImU32 border_clr      = IM_COL32(255, 255, 255, 255);

static bool behavior(ImGuiID id, const ImRect& region, 
                     const ImS32 v_min, const ImS32 v_max, 
                     ImS32* out_value, ImRect* out_thumb, 
                     ImGuiSliderFlags flags = 0,
                     bool change_on_mouse_move = false)
{
    ImGuiContext& context = *GImGui;

    const ImGuiAxis axis = (flags & ImGuiSliderFlags_Vertical) ? ImGuiAxis_Y : ImGuiAxis_X;

    const ImVec2 thumb_sz = out_thumb->GetSize();
    ImS32 v_range = (v_min < v_max ? v_max - v_min : v_min - v_max);
    const float region_usable_sz = (region.Max[axis] - region.Min[axis]);
    const float region_usable_pos_min = region.Min[axis];
    const float region_usable_pos_max = region.Max[axis];

    const float mouse_abs_pos = context.IO.MousePos[axis];
    float mouse_pos_ratio = (region_usable_sz > 0.0f) ? ImClamp((mouse_abs_pos - region_usable_pos_min) / region_usable_sz, 0.0f, 1.0f) : 0.0f;
    if (axis == ImGuiAxis_Y)
        mouse_pos_ratio = 1.0f - mouse_pos_ratio;

    // Process interacting with the slider
    ImS32 v_new = *out_value;
    bool value_changed = false;
    // wheel behavior
    ImRect mouse_wheel_responsive_region;
    if (axis == ImGuiAxis_X)
        mouse_wheel_responsive_region = ImRect(region.Min - ImVec2(thumb_sz.x / 2, 0), region.Max + ImVec2(thumb_sz.x / 2, 0));
    if (axis == ImGuiAxis_Y)
        mouse_wheel_responsive_region = ImRect(region.Min - ImVec2(0, thumb_sz.y), region.Max + ImVec2(0, thumb_sz.y));
    if (ImGui::ItemHoverable(mouse_wheel_responsive_region, id)) {
        if (change_on_mouse_move)
            v_new = v_min + (ImS32)(v_range * mouse_pos_ratio + 0.5f);
        else 
            v_new = ImClamp(*out_value + (ImS32)(context.IO.MouseWheel/* * accer*/), v_min, v_max);
    }

    // drag behavior
    if (context.ActiveId == id)
    {
        if (context.ActiveIdSource == ImGuiInputSource_Mouse)
        {
            if (context.IO.MouseReleased[0])
                ImGui::ClearActiveID();
            if (context.IO.MouseDown[0])
                v_new = v_min + (ImS32)(v_range * mouse_pos_ratio + 0.5f);
        }
    }

    // apply result, output value
    if (*out_value != v_new)
    {
        *out_value = v_new;
        value_changed = true;
    }

    // Output thumb position so it can be displayed by the caller
    const ImS32 v_clamped = (v_min < v_max) ? ImClamp(*out_value, v_min, v_max) : ImClamp(*out_value, v_max, v_min);
    float thumb_pos_ratio = v_range != 0 ? ((float)(v_clamped - v_min) / (float)v_range) : 0.0f;
    thumb_pos_ratio = axis == ImGuiAxis_Y ? 1.0f - thumb_pos_ratio : thumb_pos_ratio;
    const float thumb_pos = region_usable_pos_min + (region_usable_pos_max - region_usable_pos_min) * thumb_pos_ratio;

    ImVec2 new_thumb_center = axis == ImGuiAxis_Y ? ImVec2(out_thumb->GetCenter().x, thumb_pos) : ImVec2(thumb_pos, out_thumb->GetCenter().y);
    *out_thumb = ImRect(new_thumb_center - thumb_sz * 0.5f, new_thumb_center + thumb_sz * 0.5f);

    return value_changed;
}

ImRect ImGuiControl::DrawOptions::groove(const ImVec2& pos, const ImVec2& size, bool is_horizontal) const
{
    ImVec2 groove_start =   is_horizontal ?
                            ImVec2(pos.x + thumb_dummy_sz().x + text_dummy_sz().x, pos.y + size.y - groove_sz().y - dummy_sz().y) :
                            ImVec2(pos.x + size.x - groove_sz().x - dummy_sz().x, pos.y + text_dummy_sz().y);
    ImVec2 groove_size  =   is_horizontal ?
                            ImVec2(size.x - 2 * (thumb_dummy_sz().x + text_dummy_sz().x), groove_sz().y) :
                            ImVec2(groove_sz().x, size.y - 2 * text_dummy_sz().y);

    return ImRect(groove_start, groove_start + groove_size);
}

ImRect ImGuiControl::DrawOptions::draggable_region(const ImRect& groove, bool is_horizontal) const
{
    ImRect draggable_region =   is_horizontal ?
                                ImRect(groove.Min.x, groove.GetCenter().y, groove.Max.x, groove.GetCenter().y) :
                                ImRect(groove.GetCenter().x, groove.Min.y, groove.GetCenter().x, groove.Max.y);
    draggable_region.Expand(is_horizontal ? 
                            ImVec2(/*thumb_radius()*/0, draggable_region_sz().y) : 
                            ImVec2(draggable_region_sz().x, 0));

    return draggable_region;
}

ImRect ImGuiControl::DrawOptions::slider_line(const ImRect& draggable_region, const ImVec2& h_thumb_center, const ImVec2& l_thumb_center, bool is_horizontal) const
{
    ImVec2 mid = draggable_region.GetCenter();

    ImRect scroll_line =    is_horizontal ?
                            ImRect(ImVec2(l_thumb_center.x, mid.y - groove_sz().y / 2), ImVec2(h_thumb_center.x, mid.y + groove_sz().y / 2)) :
                            ImRect(ImVec2(mid.x - groove_sz().x / 2, h_thumb_center.y), ImVec2(mid.x + groove_sz().x / 2, l_thumb_center.y));

    return scroll_line;
}

ImGuiControl::ImGuiControl( int lowerValue,
                            int higherValue,
                            int minValue,
                            int maxValue,
                            ImGuiSliderFlags flags,
                            std::string name,
                            bool use_lower_thumb) :
    m_selection(ssUndef),
    m_name(name),
    m_lower_pos(lowerValue), 
    m_higher_pos (higherValue), 
    m_min_pos(minValue), 
    m_max_pos(maxValue),
    m_flags(flags),
    m_draw_lower_thumb(use_lower_thumb)
{
}

int ImGuiControl::GetActivePos() const
{
    return m_selection == ssLower  ? m_lower_pos : 
           m_selection == ssHigher ? m_higher_pos : -1;
}

void ImGuiControl::SetLowerPos(const int lower_pos)
{
    m_selection = ssLower;
    m_lower_pos = lower_pos;
    correct_lower_pos();
}

void ImGuiControl::SetHigherPos(const int higher_pos)
{
    m_selection = ssHigher;
    m_higher_pos = higher_pos;
    correct_higher_pos();
}

void ImGuiControl::SetSelectionSpan(const int lower_pos, const int higher_pos)
{
    m_lower_pos  = std::max(lower_pos, m_min_pos);
    m_higher_pos = std::max(std::min(higher_pos, m_max_pos), m_lower_pos);
    if (m_lower_pos < m_higher_pos)
        m_combine_thumbs = false;
}

void ImGuiControl::SetMaxPos(const int max_pos)
{
    m_max_pos = max_pos;
    correct_higher_pos();
}

void ImGuiControl::MoveActiveThumb(int delta)
{
    if (m_selection == ssUndef)
        m_selection = ssHigher;

    if (m_selection == ssLower) {
        m_lower_pos -= delta;
        correct_lower_pos();
    }
    else if (m_selection == ssHigher) {
        m_higher_pos -= delta;
        correct_higher_pos();
    }
}

void ImGuiControl::correct_lower_pos()
{
    if (m_lower_pos < m_min_pos)
        m_lower_pos = m_min_pos;
    else if (m_lower_pos > m_max_pos)
        m_lower_pos = m_max_pos;

    if ((m_lower_pos >= m_higher_pos && m_lower_pos <= m_max_pos) || m_combine_thumbs) {
        m_higher_pos = m_lower_pos;
    }
}

void ImGuiControl::correct_higher_pos()
{
    if (m_higher_pos > m_max_pos)
        m_higher_pos = m_max_pos;
    else if (m_higher_pos < m_min_pos)
        m_higher_pos = m_min_pos;

    if ((m_higher_pos <= m_lower_pos && m_higher_pos >= m_min_pos) || m_combine_thumbs) {
        m_lower_pos = m_higher_pos;
    }
}

void ImGuiControl::CombineThumbs(bool combine)
{ 
    m_combine_thumbs = combine; 
    if (combine) {
        m_selection = ssHigher;
        correct_higher_pos();
    }
    else
        ResetPositions();
}

void ImGuiControl::ResetPositions()
{
    SetLowerPos(m_min_pos);
    SetHigherPos(m_max_pos);
    m_selection == ssLower ? correct_lower_pos() : correct_higher_pos();
}

std::string ImGuiControl::get_label(int pos) const
{
    if (m_cb_get_label)
        return m_cb_get_label(pos);

    if (pos >= m_max_pos || pos < m_min_pos)
        return "ErrVal";

    return std::to_string(pos);
}

float ImGuiControl::GetPositionInRect(int pos, const ImRect& rect) const
{
    int v_min = m_min_pos;
    int v_max = m_max_pos;

    float pos_ratio = (v_max - v_min) != 0 ? ((float)(pos - v_min) / (float)(v_max - v_min)) : 0.0f;
    float thumb_pos;
    if (is_horizontal()) {
        thumb_pos = rect.Min.x + (rect.Max.x - rect.Min.x) * pos_ratio;
    }
    else {
        pos_ratio = 1.0f - pos_ratio;
        thumb_pos = rect.Min.y + (rect.Max.y - rect.Min.y) * pos_ratio;
    }
    return thumb_pos;
}

ImRect ImGuiControl::GetActiveThumbRect() const
{
    return m_selection == ssLower ? m_regions.lower_thumb : m_regions.higher_thumb;
}

bool ImGuiControl::IsLClickOnThumb()
{
    if (m_lclick_on_selected_thumb) {
        // discard left mouse click at list its value is checked to avoud reuse it on next frame
        m_lclick_on_selected_thumb = false;
        return true;
    }
    return false;
}

void ImGuiControl::draw_scroll_line(const ImRect& scroll_line, const ImRect& slideable_region)
{
    if (m_cb_draw_scroll_line)
        m_cb_draw_scroll_line(scroll_line, slideable_region);
    else
        ImGui::RenderFrame(scroll_line.Min, scroll_line.Max, thumb_bg_clr, false, m_draw_opts.rounding());
}

void ImGuiControl::draw_background(const ImRect& slideable_region)
{
    ImVec2  groove_sz       = m_draw_opts.groove_sz() * 0.55f;
    auto    groove_center   = slideable_region.GetCenter();
    ImRect  groove          = is_horizontal() ?
                              ImRect(slideable_region.Min.x, groove_center.y - groove_sz.y, slideable_region.Max.x, groove_center.y + groove_sz.y) :
                              ImRect(groove_center.x - groove_sz.x, slideable_region.Min.y, groove_center.x + groove_sz.x, slideable_region.Max.y);
    ImVec2  groove_padding  = (is_horizontal() ? ImVec2(2.0f, 2.0f) : ImVec2(3.0f, 4.0f)) * m_draw_opts.scale;

    ImRect bg_rect = groove;
    bg_rect.Expand(groove_padding);

    // draw bg of slider
    ImGui::RenderFrame(bg_rect.Min, bg_rect.Max, border_clr, false, 0.5 * bg_rect.GetWidth());
    // draw bg of scroll
    ImGui::RenderFrame(groove.Min, groove.Max, groove_bg_clr, false, 0.5 * groove.GetWidth());
}

void ImGuiControl::draw_label(std::string label, const ImRect& thumb, bool is_mirrored /*= false*/, bool with_border /*= false*/)
{
    if (label.empty() || label == "ErrVal")
        return;

    const ImVec2 thumb_center   = thumb.GetCenter();
    ImVec2 text_padding         = m_draw_opts.text_padding();
    float  rounding             = m_draw_opts.rounding();

    float triangle_offset_x = 9.f * m_draw_opts.scale;
    float triangle_offset_y = 8.f * m_draw_opts.scale;

    ImVec2 text_content_size    = ImGui::CalcTextSize(label.c_str());
    ImVec2 text_size    = text_content_size + text_padding * 2;
    ImVec2 text_start   = is_horizontal() ?
                        ImVec2(thumb.Max.x + triangle_offset_x, thumb_center.y - text_size.y) : 
                        ImVec2(thumb.Min.x - text_size.x - triangle_offset_x, thumb_center.y - text_size.y) ;

    if (is_mirrored)
        text_start   = is_horizontal() ?
                        ImVec2(thumb.Min.x - text_size.x - triangle_offset_x, thumb_center.y - text_size.y) :
                        ImVec2(thumb.Min.x - text_size.x - triangle_offset_x, thumb_center.y) ;

    ImRect text_rect(text_start, text_start + text_size);

    if (with_border) {

        float  rounding_b = 0.75f * rounding;
        
        ImRect text_rect_b(text_rect);
        text_rect_b.Expand(ImVec2(rounding_b, rounding_b));

        float triangle_offset_x_b = triangle_offset_x + rounding_b;
        float triangle_offset_y_b = triangle_offset_y + rounding_b;

        ImVec2 pos_1 = is_horizontal() ?
            ImVec2(text_rect_b.Min.x + rounding_b, text_rect_b.Max.y) :
            ImVec2(text_rect_b.Max.x - rounding_b, text_rect_b.Max.y);
        ImVec2 pos_2 = is_horizontal() ? pos_1 - ImVec2(triangle_offset_x_b, 0.f) : pos_1 - ImVec2(0.f, triangle_offset_y_b);
        ImVec2 pos_3 = is_horizontal() ? pos_1 - ImVec2(0.f, triangle_offset_y_b) : pos_1 + ImVec2(triangle_offset_x_b, 0.f);

        if (is_mirrored) {
            pos_1 = is_horizontal() ?
                ImVec2(text_rect_b.Max.x - rounding_b - 1, text_rect_b.Max.y - 1) :
                ImVec2(text_rect_b.Max.x - rounding_b, text_rect_b.Min.y);
            pos_2 = is_horizontal() ? pos_1 + ImVec2(triangle_offset_x_b, 0.f) : pos_1 + ImVec2(0.f, triangle_offset_y_b);
            pos_3 = is_horizontal() ? pos_1 - ImVec2(0.f, triangle_offset_y_b) : pos_1 + ImVec2(triangle_offset_x_b, 0.f);
        }

        ImGui::RenderFrame(text_rect_b.Min, text_rect_b.Max, thumb_bg_clr, true, rounding);
        ImGui::GetCurrentWindow()->DrawList->AddTriangleFilled(pos_1, pos_2, pos_3, thumb_bg_clr);
    }

    ImVec2 pos_1 = is_horizontal() ?
                   ImVec2(text_rect.Min.x + rounding, text_rect.Max.y) :
                   ImVec2(text_rect.Max.x - rounding, text_rect.Max.y);
    ImVec2 pos_2 = is_horizontal() ? pos_1 - ImVec2(triangle_offset_x, 0.f) : pos_1 - ImVec2(0.f, triangle_offset_y);
    ImVec2 pos_3 = is_horizontal() ? pos_1 - ImVec2(0.f, triangle_offset_y) : pos_1 + ImVec2(triangle_offset_x, 0.f);

    if (is_mirrored) {
        pos_1 = is_horizontal() ?
                ImVec2(text_rect.Max.x - rounding-1, text_rect.Max.y-1) :
                ImVec2(text_rect.Max.x - rounding, text_rect.Min.y);
        pos_2 = is_horizontal() ? pos_1 + ImVec2(triangle_offset_x, 0.f) : pos_1 + ImVec2(0.f, triangle_offset_y);
        pos_3 = is_horizontal() ? pos_1 - ImVec2(0.f, triangle_offset_y) : pos_1 + ImVec2(triangle_offset_x, 0.f);
    }

    ImGui::RenderFrame(text_rect.Min, text_rect.Max, tooltip_bg_clr, true, rounding);
    ImGui::GetCurrentWindow()->DrawList->AddTriangleFilled(pos_1, pos_2, pos_3, tooltip_bg_clr);
    ImGui::RenderText(text_start + text_padding, label.c_str());
};

void ImGuiControl::draw_thumb(const ImVec2& center, bool mark/* = false*/)
{
    const float line_width  = 1.5f * m_draw_opts.scale;
    const float radius      = m_draw_opts.thumb_radius();
    const float line_offset = 0.5f * radius;
    const float rounding    = 1.5f * m_draw_opts.rounding();

    const float hexagon_angle = is_horizontal() ? 0.f : IM_PI * 0.5f;

    ImGuiPureWrap::draw_hexagon(center, radius,              border_clr,    hexagon_angle, rounding);
    ImGuiPureWrap::draw_hexagon(center, radius - line_width, thumb_bg_clr,  hexagon_angle, rounding);

    if (mark) {
        ImGuiWindow* window = ImGui::GetCurrentWindow();
        window->DrawList->AddLine(center + ImVec2(-line_offset, 0.0f), center + ImVec2(line_offset, 0.0f), border_clr, line_width);
        window->DrawList->AddLine(center + ImVec2(0.0f, -line_offset), center + ImVec2(0.0f, line_offset), border_clr, line_width);
    }
}

void ImGuiControl::apply_regions(int higher_pos, int lower_pos, const ImRect& draggable_region)
{
    ImVec2  mid             = draggable_region.GetCenter();
    float   thumb_radius    = m_draw_opts.thumb_radius();

    // set slideable region
    m_regions.higher_slideable_region = is_horizontal() ? 
                                        ImRect(draggable_region.Min + ImVec2(m_draw_lower_thumb ? thumb_radius : 0, 0), draggable_region.Max) :
                                        ImRect(draggable_region.Min, draggable_region.Max - ImVec2(0, m_combine_thumbs ? 0 : thumb_radius));
    m_regions.lower_slideable_region  = is_horizontal() ? 
                                        ImRect(draggable_region.Min, draggable_region.Max - ImVec2(thumb_radius, 0)) : 
                                        ImRect(draggable_region.Min + ImVec2(0, thumb_radius), draggable_region.Max);

    // initialize the thumbs.
    float higher_thumb_pos = GetPositionInRect(higher_pos, m_regions.higher_slideable_region);
    m_regions.higher_thumb =    is_horizontal() ? 
                                ImRect(higher_thumb_pos - thumb_radius, mid.y - thumb_radius, higher_thumb_pos + thumb_radius, mid.y + thumb_radius) : 
                                ImRect(mid.x - thumb_radius, higher_thumb_pos - thumb_radius, mid.x + thumb_radius, higher_thumb_pos + thumb_radius);

    float  lower_thumb_pos = GetPositionInRect(lower_pos, m_regions.lower_slideable_region);
    m_regions.lower_thumb  =    is_horizontal() ? 
                                ImRect(lower_thumb_pos - thumb_radius, mid.y - thumb_radius, lower_thumb_pos + thumb_radius, mid.y + thumb_radius) :
                                ImRect(mid.x - thumb_radius, lower_thumb_pos - thumb_radius, mid.x + thumb_radius, lower_thumb_pos + thumb_radius);
}

void ImGuiControl::check_and_correct_thumbs(int* higher_pos, int* lower_pos)
{
    if (!m_draw_lower_thumb || m_combine_thumbs)
        return;

    const ImVec2 higher_thumb_center = m_regions.higher_thumb.GetCenter();
    const ImVec2 lower_thumb_center  = m_regions.lower_thumb.GetCenter();
    const float thumb_radius             = m_draw_opts.thumb_radius();

    const float higher_thumb_center_pos = is_horizontal() ? higher_thumb_center.x : higher_thumb_center.y;
    const float lower_thumb_center_pos  = is_horizontal() ? lower_thumb_center.x  : lower_thumb_center.y;

    if (is_horizontal()) {
        if (lower_thumb_center_pos + thumb_radius > higher_thumb_center_pos) { 
            if (m_selection == ssHigher) {
                m_regions.higher_thumb = m_regions.lower_thumb;
                m_regions.higher_thumb.TranslateX(thumb_radius);
                *lower_pos = *higher_pos;
            }
            else {
                m_regions.lower_thumb = m_regions.higher_thumb;
                m_regions.lower_thumb.TranslateX(-thumb_radius);
                *higher_pos = *lower_pos;
            }
        }
    }
    else {
        if (higher_thumb_center_pos + thumb_radius > lower_thumb_center_pos) {
            if (m_selection == ssHigher) {
                m_regions.lower_thumb = m_regions.higher_thumb;
                m_regions.lower_thumb.TranslateY(thumb_radius);
                *lower_pos = *higher_pos;
            }        
            else {
                m_regions.higher_thumb = m_regions.lower_thumb;
                m_regions.higher_thumb.TranslateY(-thumb_radius);
                *higher_pos = *lower_pos;
            }
        }
    }
}

bool ImGuiControl::draw_slider( int* higher_pos, int* lower_pos, 
                                std::string& higher_label, std::string& lower_label, 
                                const ImVec2& pos, const ImVec2& size, float scale)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& context = *GImGui;
    const ImGuiID id = window->GetID(m_name.c_str());

    const ImRect item_size(pos, pos + size);
    ImGui::ItemSize(item_size);

    // get slider groove size
    ImRect groove = m_draw_opts.groove(pos, size, is_horizontal());

    // get active(draggable) region.
    ImRect draggable_region = m_draw_opts.draggable_region(groove, is_horizontal());

    if (ImGui::ItemHoverable(draggable_region, id) && context.IO.MouseDown[0]) {
        ImGui::SetActiveID(id, window);
        ImGui::SetFocusID(id, window);
        ImGui::FocusWindow(window);
    }
    
    // set slideable regions and thumbs.
    apply_regions(*higher_pos, *lower_pos, draggable_region);

    // select and mark higher thumb by default
    if (m_selection == ssUndef)
        m_selection = ssHigher;

    // Processing interacting

    if (ImGui::ItemHoverable(m_regions.higher_thumb, id) && context.IO.MouseClicked[0])
        m_selection = ssHigher;

    if (m_draw_lower_thumb && !m_combine_thumbs &&
        ImGui::ItemHoverable(m_regions.lower_thumb, id) && context.IO.MouseClicked[0])
        m_selection = ssLower;

    // update thumb position
    bool pos_changed = false;
    if (m_selection == ssHigher) {
        pos_changed = behavior(id, m_regions.higher_slideable_region, m_min_pos, m_max_pos,
                                 higher_pos, &m_regions.higher_thumb, m_flags);
    }
    else if (m_draw_lower_thumb && !m_combine_thumbs) {
        pos_changed = behavior(id, m_regions.lower_slideable_region, m_min_pos, m_max_pos,
                                 lower_pos, &m_regions.lower_thumb, m_flags);
    }

    // check thumbs poss and correct them if needed
    check_and_correct_thumbs(higher_pos, lower_pos);

    const ImRect& slideable_region  = m_selection == ssHigher ? m_regions.higher_slideable_region : m_regions.lower_slideable_region;
    const ImRect& active_thumb      = m_selection == ssHigher ? m_regions.higher_thumb            : m_regions.lower_thumb;

    bool show_move_label = false;
    ImRect mouse_pos_rc = active_thumb;
    if (!pos_changed && ImGui::ItemHoverable(item_size, id) && !ImGui::IsMouseDragging(0)) {
        behavior(id, slideable_region, m_min_pos, m_max_pos,
                 &m_mouse_pos, &mouse_pos_rc, m_flags, true);
        show_move_label = true;
    }

    // detect right click on selected thumb
    if (ImGui::ItemHoverable(active_thumb, id) && context.IO.MouseClicked[1])
        m_rclick_on_selected_thumb = true;
    if ((!ImGui::ItemHoverable(active_thumb, id) && context.IO.MouseClicked[1]) ||
        context.IO.MouseClicked[0])
        m_rclick_on_selected_thumb = false;

    // detect left click on selected thumb
    if (ImGui::ItemHoverable(active_thumb, id) && !pos_changed) {
        ImVec2 active_thumb_center = active_thumb.GetCenter();
        if (context.IO.MouseClicked[0])
            m_active_thumb_center_on_lcklick = active_thumb_center;
        if (context.IO.MouseReleased[0] && 
            (m_active_thumb_center_on_lcklick.y == active_thumb_center.y) && 
            (m_active_thumb_center_on_lcklick.x == active_thumb_center.x)     )
            m_lclick_on_selected_thumb = true;
    }

    // render slider

    ImVec2 higher_thumb_center = m_regions.higher_thumb.GetCenter();
    ImVec2 lower_thumb_center  = m_regions.lower_thumb.GetCenter();

    ImRect scroll_line = m_draw_opts.slider_line(slideable_region, higher_thumb_center, lower_thumb_center, is_horizontal());

    if (m_cb_extra_draw)
        m_cb_extra_draw(slideable_region);

    // draw background
    draw_background(slideable_region);
    // draw scroll line
    draw_scroll_line(m_combine_thumbs ? groove : scroll_line, slideable_region);

    // draw thumbs with label
    draw_thumb(higher_thumb_center, m_selection == ssHigher && m_draw_lower_thumb);
    draw_label(higher_label, m_regions.higher_thumb);

    if (m_draw_lower_thumb && !m_combine_thumbs) {
        ImVec2 text_size = ImGui::CalcTextSize(lower_label.c_str()) + m_draw_opts.text_padding() * 2.f;
        const bool mirror_label = is_horizontal() ? (higher_thumb_center.x - lower_thumb_center.x < text_size.x) :
                                                    (lower_thumb_center.y - higher_thumb_center.y < text_size.y);

        draw_thumb(lower_thumb_center, m_selection == ssLower);
        draw_label(lower_label, m_regions.lower_thumb, mirror_label);
    }

    // draw label on mouse move
    if (show_move_label)
        draw_label(get_label_on_move(m_mouse_pos), mouse_pos_rc, false, true);

    return pos_changed;
}

bool ImGuiControl::render()
{
    bool result = false;

    ImGui::PushStyleVar(ImGuiStyleVar_::ImGuiStyleVar_WindowBorderSize, 0);
    ImGui::PushStyleVar(ImGuiStyleVar_::ImGuiStyleVar_FramePadding, ImVec2(0.0f, 0.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_::ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

    ImGui::PushStyleColor(ImGuiCol_::ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_Text));

    int windows_flag =    ImGuiWindowFlags_NoTitleBar
                        | ImGuiWindowFlags_NoCollapse
                        | ImGuiWindowFlags_NoMove
                        | ImGuiWindowFlags_NoResize
                        | ImGuiWindowFlags_NoScrollbar
                        | ImGuiWindowFlags_NoScrollWithMouse;

    ImGuiPureWrap::set_next_window_pos(m_pos.x, m_pos.y, ImGuiCond_Always);
    ImGuiPureWrap::begin(m_name, windows_flag);

    float scale = 1.f;

    int         higher_pos        = m_higher_pos;
    int         lower_pos         = m_lower_pos;
    std::string higher_label        = get_label(m_higher_pos);
    std::string lower_label         = get_label(m_lower_pos);
    int         temp_higher_pos   = m_higher_pos;
    int         temp_lower_pos    = m_lower_pos;

    if (draw_slider(&higher_pos, &lower_pos, higher_label, lower_label, m_pos, m_size, scale)) {
        if (temp_higher_pos != higher_pos) {
            m_higher_pos = higher_pos;
            if (m_combine_thumbs)
                m_lower_pos = m_higher_pos;
        }
        if (temp_lower_pos != lower_pos)
            m_lower_pos = lower_pos;
        result = true;
    }

    ImGuiPureWrap::end();

    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(3);

    return result;
}

} // DoubleSlider

