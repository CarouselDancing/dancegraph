#pragma once

#include <magic_enum/magic_enum.hpp>
#include <imgui/imgui.h>

namespace imgui
{
	void GlobalLogLevel();

    template<class T>
    bool Enum(const char* label, T& selected)
    {
        auto selected_prev = selected;
        auto num = magic_enum::enum_names<T>().size();
        // later in your code...
        if (ImGui::BeginCombo(label, magic_enum::enum_name(selected).data())) {
            for (int i = 0; i < num; ++i) {
                const bool isSelected = ((int)selected == i);
                if (ImGui::Selectable(magic_enum::enum_name((T)i).data(), isSelected)) {
                    selected = (T)i;
                }

                // Set the initial focus when opening the combo
                // (scrolling + keyboard navigation focus)
                if (isSelected) {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }
        return selected_prev != selected;
    }
}