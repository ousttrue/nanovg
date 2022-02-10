#pragma once
// Backend API
bool nvg_ImplOpenGL3_Init(struct NVGcontext *vg);
void nvg_ImplOpenGL3_Shutdown();
void nvg_ImplOpenGL3_RenderDrawData(struct NVGdrawData *draw_data);
