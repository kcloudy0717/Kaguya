#include "ConsoleWindow.h"

void ConsoleWindow::OnRender()
{
	//// Options menu
	// if (ImGui::BeginPopup("Options"))
	//{
	//	ImGui::Checkbox("Auto-scroll", &AutoScroll);
	//	ImGui::EndPopup();
	//}

	//// Main window
	// if (ImGui::Button("Options"))
	//	ImGui::OpenPopup("Options");
	// ImGui::SameLine();
	// bool bClear = ImGui::Button("Clear");
	// ImGui::SameLine();
	// bool bCopy = ImGui::Button("Copy");
	// ImGui::SameLine();
	// Log::Filter.Draw("Filter", -100.0f);

	// ImGui::Separator();
	// ImGui::BeginChild("scrolling", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);

	// if (bClear)
	//	Clear();
	// if (bCopy)
	//	ImGui::LogToClipboard();

	// ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
	// const char* BufBegin = Log::Buf.begin();
	// const char* BufEnd	 = Log::Buf.end();
	// if (Log::Filter.IsActive())
	//{
	//	// In this example we don't use the clipper when Filter is enabled.
	//	// This is because we don't have a random access on the result on our filter.
	//	// A real application processing logs with ten of thousands of entries may want to store the result of
	//	// search/filter.. especially if the filtering function is not trivial (e.g. reg-exp).
	//	for (int LineNumber = 0; LineNumber < Log::LineOffsets.Size; LineNumber++)
	//	{
	//		const char* LineStart = BufBegin + Log::LineOffsets[LineNumber];
	//		const char* LineEnd =
	//			(LineNumber + 1 < Log::LineOffsets.Size) ? (BufBegin + Log::LineOffsets[LineNumber + 1] - 1) : BufEnd;
	//		if (Log::Filter.PassFilter(LineStart, LineEnd))
	//			ImGui::TextUnformatted(LineStart, LineEnd);
	//	}
	//}
	// else
	//{
	//	// The simplest and easy way to display the entire buffer:
	//	//   ImGui::TextUnformatted(buf_begin, buf_end);
	//	// And it'll just work. TextUnformatted() has specialization for large blob of text and will fast-forward
	//	// to skip non-visible lines. Here we instead demonstrate using the clipper to only process lines that are
	//	// within the visible area.
	//	// If you have tens of thousands of items and their processing cost is non-negligible, coarse clipping them
	//	// on your side is recommended. Using ImGuiListClipper requires
	//	// - A) random access into your data
	//	// - B) items all being the  same height,
	//	// both of which we can handle since we an array pointing to the beginning of each line of text.
	//	// When using the filter (in the block of code above) we don't have random access into the data to display
	//	// anymore, which is why we don't use the clipper. Storing or skimming through the search result would make
	//	// it possible (and would be recommended if you want to search through tens of thousands of entries).
	//	ImGuiListClipper ListClipper;
	//	ListClipper.Begin(Log::LineOffsets.Size);
	//	while (ListClipper.Step())
	//	{
	//		for (int LineNumber = ListClipper.DisplayStart; LineNumber < ListClipper.DisplayEnd; LineNumber++)
	//		{
	//			const char* LineStart = BufBegin + Log::LineOffsets[LineNumber];
	//			const char* LineEnd	  = (LineNumber + 1 < Log::LineOffsets.Size)
	//										? (BufBegin + Log::LineOffsets[LineNumber + 1] - 1)
	//										: BufEnd;
	//			ImGui::TextUnformatted(LineStart, LineEnd);
	//		}
	//	}
	//	ListClipper.End();
	//}
	// ImGui::PopStyleVar();

	// if (AutoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
	//	ImGui::SetScrollHereY(1.0f);

	// ImGui::EndChild();
}

void ConsoleWindow::Clear()
{
	/*Log::Buf.clear();
	Log::LineOffsets.clear();
	Log::LineOffsets.push_back(0);*/
}
