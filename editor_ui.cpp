/*==============================================================================

　　  ImguiのUI作り[editor_ui.cpp]
                                                         Author : Kouki Tanaka
                                                         Date   : 2026/01/04
--------------------------------------------------------------------------------

==============================================================================*/
#include"editor_ui.h"
#include "imgui.h"
#include "stage01_manage.h"
#include "stage_cube.h"
#include "player_sensors.h"
#include "player.h"
#include <cstdio>
#include<algorithm>
#include <sstream>
#include <iomanip>
#include <cfloat>
#include <cstring>
#include <vector>
#include <cmath>


static int s_selected = -1;

static std::string s_exportText;
static bool s_exportCopied = false;

static char s_jsonPath[260] = "stage01.json";
static char s_ioStatus[128] = "";

// ===== Kind Editor state =====
static int s_selectedKind = 0;
static int s_selectedFace = 0;
static bool s_editNormals = false;
static const char* kFaceNames[] = { "Front","Back","Left","Right","Top","Bottom" };

static bool s_pathInited = false;

// ===== Duplicate/Snap state =====
static float s_snap = 1.0f;
static float s_dupOffset[3] = { 1.0f, 0.0f, 0.0f }; // in block units
static int   s_dupCount = 1;
static bool  s_dupOffsetUseSnap = false; // if true: offset * Snap


// ===== MotionLab =====
struct MotionLabResult
{
    char  name[64]{};
    float dt = 0.0f;

    // measured (all in BLOCK units except time)
    float distXZ = 0.0f;       // horizontal distance (blocks)
    float distAlong = 0.0f;    // forward-axis component (blocks)
    float maxY = 0.0f;         // max height delta from startY (blocks)
    float hangTime = 0.0f;     // airtime (sec)

    // targets
    float targetDist = 0.0f;   // (blocks) use with distAlong
    float targetHang = 0.0f;   // (sec)
    float targetMaxY = 0.0f;   // (blocks)

    // errors (measured - target)
    float errDist = 0.0f;
    float errHang = 0.0f;
    float errMaxY = 0.0f;

    // speed logs (blocks/sec)
    float vMaxXZ = 0.0f;
    float vAvgXZ = 0.0f;
};

enum class MotionLabMode
{
    Idle,
    Run,
    Jump
};

static struct MotionLabState
{
    bool open = true;

    // common
    MotionLabMode mode = MotionLabMode::Idle;
    double t = 0.0;
    DirectX::XMFLOAT3 startPos{};
    float startY = 0.0f;
    float maxYWorld = 0.0f;

    // units
    float blockSize = 1.0f; // 1 block = ? world units

    // teleport
    bool useTeleport = true;
    float teleportPos[3] = { 0.0f, 3.0f, 0.0f };

    // forward axis (world fixed)
    // 0: +Z, 1: +X
    int forwardAxis = 0;

    // Run test
    float runDuration = 0.30f;
    float runTargetDist = 2.0f;
    float runMoveY = 1.0f;
    float runMoveX = 0.0f;
    bool  runDash = false;

    // Jump test
    float jumpHold = 0.08f;
    float jumpTimeout = 2.0f;
    float jumpTargetHang = 0.60f;
    float jumpTargetMaxY = 2.50f; // blocks (delta from startY)
    bool  jumpWithMove = false;
    float jumpMoveY = 1.0f;
    float jumpMoveX = 0.0f;

    // runtime speed logging
    DirectX::XMFLOAT3 prevPos{};
    bool  hasPrevPos = false;
    float vMaxXZ = 0.0f;     // blocks/sec
    float vSumXZ = 0.0f;     // blocks/sec
    int   vSamples = 0;

    // airtime measurement for Jump
    bool   prevGrounded = true;
    bool   airStarted = false;
    double airStartT = 0.0;
    double airTime = 0.0;

    // results
    std::vector<MotionLabResult> results;

    // CSV export
    char csvPath[260] = "motionlab.csv";
    std::vector<char> csvBuf;     // null-terminated
    bool csvCopied = false;
    bool csvSaved = false;
    char csvStatus[128] = "";

} s_motionLab;


static float MotionLab_SafeBlockSize()
{
    return (s_motionLab.blockSize > 0.0f) ? s_motionLab.blockSize : 1.0f;
}

static DirectX::XMFLOAT3 MotionLab_GetForwardAxis()
{
    if (s_motionLab.forwardAxis == 1) return { 1,0,0 };
    return { 0,0,1 };
}

static float MotionLab_DotXZ(const DirectX::XMFLOAT3& a, const DirectX::XMFLOAT3& b)
{
    return a.x * b.x + a.z * b.z;
}

static float MotionLab_LenXZ(const DirectX::XMFLOAT3& v)
{
    return sqrtf(v.x * v.x + v.z * v.z);
}

static void MotionLab_Stop()
{
    Player_SetInputOverride(false, nullptr);
    s_motionLab.mode = MotionLabMode::Idle;
    s_motionLab.t = 0.0;
}

static void MotionLab_ResetRuntimeStats()
{
    s_motionLab.hasPrevPos = true;
    s_motionLab.prevPos = Player_GetPosition();

    s_motionLab.vMaxXZ = 0.0f;
    s_motionLab.vSumXZ = 0.0f;
    s_motionLab.vSamples = 0;

    s_motionLab.prevGrounded = Player_IsGrounded();
    s_motionLab.airStarted = false;
    s_motionLab.airStartT = 0.0;
    s_motionLab.airTime = 0.0;
}

static void MotionLab_StartCommon(const char* label)
{
    (void)label;

    if (s_motionLab.useTeleport)
    {
        DirectX::XMFLOAT3 p = { s_motionLab.teleportPos[0], s_motionLab.teleportPos[1], s_motionLab.teleportPos[2] };
        Player_DebugTeleport(p, true);
    }

    s_motionLab.startPos = Player_GetPosition();
    s_motionLab.startY = s_motionLab.startPos.y;
    s_motionLab.maxYWorld = s_motionLab.startPos.y;
    s_motionLab.t = 0.0;

    MotionLab_ResetRuntimeStats();

    // start with neutral override for safety
    PlayerInput neutral{};
    Player_SetInputOverride(true, &neutral);
}

static void MotionLab_PushResultFront(const MotionLabResult& rIn)
{
    s_motionLab.results.insert(s_motionLab.results.begin(), rIn);
    if (s_motionLab.results.size() > 30) s_motionLab.results.pop_back();
}

static void MotionLab_FinishRun()
{
    const float bs = MotionLab_SafeBlockSize();

    DirectX::XMFLOAT3 endPos = Player_GetPosition();
    DirectX::XMFLOAT3 d{ endPos.x - s_motionLab.startPos.x, endPos.y - s_motionLab.startPos.y, endPos.z - s_motionLab.startPos.z };

    MotionLabResult r{};
    strcpy_s(r.name, "Run");
    r.dt = (float)s_motionLab.t;

    r.distXZ = MotionLab_LenXZ(d) / bs;
    r.distAlong = MotionLab_DotXZ(d, MotionLab_GetForwardAxis()) / bs;

    r.targetDist = s_motionLab.runTargetDist;
    r.errDist = r.distAlong - r.targetDist;

    // speed logs (blocks/sec)
    r.vMaxXZ = s_motionLab.vMaxXZ;
    r.vAvgXZ = (r.dt > 0.0f) ? (r.distXZ / r.dt) : 0.0f;

    MotionLab_PushResultFront(r);
    MotionLab_Stop();
}

static void MotionLab_FinishJump()
{
    const float bs = MotionLab_SafeBlockSize();

    DirectX::XMFLOAT3 endPos = Player_GetPosition();
    DirectX::XMFLOAT3 d{ endPos.x - s_motionLab.startPos.x, endPos.y - s_motionLab.startPos.y, endPos.z - s_motionLab.startPos.z };

    MotionLabResult r{};
    strcpy_s(r.name, "Jump");
    r.dt = (float)s_motionLab.t;

    r.distXZ = MotionLab_LenXZ(d) / bs;
    r.distAlong = MotionLab_DotXZ(d, MotionLab_GetForwardAxis()) / bs;

    // maxY is delta from startY (blocks)
    r.maxY = (s_motionLab.maxYWorld - s_motionLab.startY) / bs;

    // hangTime is airtime (sec)
    if (s_motionLab.airStarted)
        r.hangTime = (float)s_motionLab.airTime;
    else
        r.hangTime = (float)s_motionLab.t;

    r.targetHang = s_motionLab.jumpTargetHang;
    r.targetMaxY = s_motionLab.jumpTargetMaxY;

    r.errHang = r.hangTime - r.targetHang;
    r.errMaxY = r.maxY - r.targetMaxY;

    r.vMaxXZ = s_motionLab.vMaxXZ;
    r.vAvgXZ = (r.dt > 0.0f) ? (r.distXZ / r.dt) : 0.0f;

    MotionLab_PushResultFront(r);
    MotionLab_Stop();
}

static std::string MotionLab_BuildCsvText()
{
    std::ostringstream oss;
    oss << "Name,dt,distXZ,distAlong,targetDist,errDist,maxY,targetMaxY,errMaxY,hang,targetHang,errHang,vMaxXZ,vAvgXZ\n";
    oss << std::fixed << std::setprecision(6);

    for (const auto& r : s_motionLab.results)
    {
        oss
            << r.name << ","
            << r.dt << ","
            << r.distXZ << ","
            << r.distAlong << ","
            << r.targetDist << ","
            << r.errDist << ","
            << r.maxY << ","
            << r.targetMaxY << ","
            << r.errMaxY << ","
            << r.hangTime << ","
            << r.targetHang << ","
            << r.errHang << ","
            << r.vMaxXZ << ","
            << r.vAvgXZ
            << "\n";
    }
    return oss.str();
}

static void MotionLab_RebuildCsvBuffer()
{
    const std::string csv = MotionLab_BuildCsvText();
    s_motionLab.csvBuf.assign(csv.begin(), csv.end());
    s_motionLab.csvBuf.push_back('\0');
    s_motionLab.csvCopied = false;
    s_motionLab.csvSaved = false;
    s_motionLab.csvStatus[0] = '\0';
}


static void MotionLab_Update(double dt)
{
    if (s_motionLab.mode == MotionLabMode::Idle)
        return;

    s_motionLab.t += dt;

    // track maxY & speed (per frame)
    {
        DirectX::XMFLOAT3 p = Player_GetPosition();
        if (p.y > s_motionLab.maxYWorld) s_motionLab.maxYWorld = p.y;

        if (dt > 0.0 && s_motionLab.hasPrevPos)
        {
            const float bs = MotionLab_SafeBlockSize();

            DirectX::XMFLOAT3 dp{ p.x - s_motionLab.prevPos.x, p.y - s_motionLab.prevPos.y, p.z - s_motionLab.prevPos.z };
            const float vXZ = (MotionLab_LenXZ(dp) / (float)dt) / bs; // blocks/sec

            if (vXZ > s_motionLab.vMaxXZ) s_motionLab.vMaxXZ = vXZ;
            s_motionLab.vSumXZ += vXZ;
            s_motionLab.vSamples += 1;
        }

        s_motionLab.prevPos = p;
        s_motionLab.hasPrevPos = true;
    }

    // input override (per frame)
    PlayerInput in{};

    if (s_motionLab.mode == MotionLabMode::Run)
    {
        in.moveY = s_motionLab.runMoveY;
        in.moveX = s_motionLab.runMoveX;
        in.dash = s_motionLab.runDash;

        Player_SetInputOverride(true, &in);

        if (s_motionLab.t >= s_motionLab.runDuration)
        {
            MotionLab_FinishRun();
        }
    }
    else if (s_motionLab.mode == MotionLabMode::Jump)
    {
        if (s_motionLab.jumpWithMove)
        {
            in.moveY = s_motionLab.jumpMoveY;
            in.moveX = s_motionLab.jumpMoveX;
        }

        // hold jump for a while
        in.jump = (s_motionLab.t <= s_motionLab.jumpHold);
        Player_SetInputOverride(true, &in);

        const bool grounded = Player_IsGrounded();

        // detect takeoff
        if (!s_motionLab.airStarted && s_motionLab.prevGrounded && !grounded)
        {
            s_motionLab.airStarted = true;
            s_motionLab.airStartT = s_motionLab.t;
        }

        // detect landing after takeoff
        if (s_motionLab.airStarted && grounded)
        {
            s_motionLab.airTime = s_motionLab.t - s_motionLab.airStartT;
            MotionLab_FinishJump();
            return;
        }

        s_motionLab.prevGrounded = grounded;

        // timeout safety
        if (s_motionLab.t >= s_motionLab.jumpTimeout)
        {
            if (s_motionLab.airStarted)
                s_motionLab.airTime = s_motionLab.t - s_motionLab.airStartT;
            MotionLab_FinishJump();
        }
    }
}

static void MotionLab_Draw(double dt)
{
    // ★重要：ウィンドウが折り畳まれても計測が進むように、Begin前に更新
    MotionLab_Update(dt);

    // closeされたら安全に停止
    if (!s_motionLab.open)
    {
        if (s_motionLab.mode != MotionLabMode::Idle)
            MotionLab_Stop();
        return;
    }

    if (!ImGui::Begin("Motion Lab", &s_motionLab.open))
    {
        ImGui::End();
        return;
    }

    // safety: Idleなら上書き解除
    if (s_motionLab.mode == MotionLabMode::Idle && Player_IsInputOverrideEnabled())
        Player_SetInputOverride(false, nullptr);

    ImGui::Text("Mode: %s", (s_motionLab.mode == MotionLabMode::Idle) ? "Idle" :
        (s_motionLab.mode == MotionLabMode::Run) ? "Run" : "Jump");

    if (s_motionLab.mode != MotionLabMode::Idle)
    {
        ImGui::SameLine();
        if (ImGui::Button("STOP"))
            MotionLab_Stop();
    }

    ImGui::Separator();

    // Units
    ImGui::DragFloat("Block Size (world units)", &s_motionLab.blockSize, 0.01f, 0.01f, 100.0f);

    // Teleport
    ImGui::Checkbox("Teleport on Start", &s_motionLab.useTeleport);
    ImGui::DragFloat3("Teleport Pos", s_motionLab.teleportPos, 0.1f);

    // Axis
    ImGui::Combo("Forward Axis", &s_motionLab.forwardAxis, "World +Z\0World +X\0");

    ImGui::Separator();

    // ===== Run =====
    ImGui::TextUnformatted("Run Test");
    ImGui::DragFloat("Duration (sec)", &s_motionLab.runDuration, 0.01f, 0.01f, 10.0f);
    ImGui::DragFloat("Target DistAlong (blocks)", &s_motionLab.runTargetDist, 0.01f, -999.0f, 999.0f);
    ImGui::DragFloat("Input moveY", &s_motionLab.runMoveY, 0.01f, -1.0f, 1.0f);
    ImGui::DragFloat("Input moveX", &s_motionLab.runMoveX, 0.01f, -1.0f, 1.0f);
    ImGui::Checkbox("Dash", &s_motionLab.runDash);

    if (ImGui::Button("START Run"))
    {
        MotionLab_StartCommon("Run");
        s_motionLab.mode = MotionLabMode::Run;
    }

    ImGui::Separator();

    // ===== Jump =====
    ImGui::TextUnformatted("Jump Test");
    ImGui::DragFloat("Hold (sec)", &s_motionLab.jumpHold, 0.01f, 0.0f, 1.0f);
    ImGui::DragFloat("Timeout (sec)", &s_motionLab.jumpTimeout, 0.01f, 0.1f, 10.0f);

    ImGui::DragFloat("Target HangTime (sec)", &s_motionLab.jumpTargetHang, 0.01f, 0.0f, 10.0f);
    ImGui::DragFloat("Target MaxY (blocks, delta)", &s_motionLab.jumpTargetMaxY, 0.01f, -999.0f, 999.0f);

    ImGui::Checkbox("Move while Jump", &s_motionLab.jumpWithMove);
    if (s_motionLab.jumpWithMove)
    {
        ImGui::DragFloat("Jump moveY", &s_motionLab.jumpMoveY, 0.01f, -1.0f, 1.0f);
        ImGui::DragFloat("Jump moveX", &s_motionLab.jumpMoveX, 0.01f, -1.0f, 1.0f);
    }

    if (ImGui::Button("START Jump"))
    {
        MotionLab_StartCommon("Jump");
        s_motionLab.mode = MotionLabMode::Jump;
    }

    ImGui::Separator();

    // ===== Live =====
    {
        DirectX::XMFLOAT3 p = Player_GetPosition();
        const float bs = MotionLab_SafeBlockSize();
        ImGui::Text("Player Pos: (%.3f, %.3f, %.3f)", p.x, p.y, p.z);
        ImGui::Text("Grounded: %s", Player_IsGrounded() ? "true" : "false");
        ImGui::Text("t=%.3f  vMaxXZ=%.3f (blocks/s)", (float)s_motionLab.t, s_motionLab.vMaxXZ);
        ImGui::Text("maxY(delta)=%.3f (blocks)", (s_motionLab.maxYWorld - s_motionLab.startY) / bs);
    }

    // ===== Results =====
    if (ImGui::CollapsingHeader("Results", ImGuiTreeNodeFlags_DefaultOpen))
    {
        if (ImGui::Button("Clear Results"))
        {
            s_motionLab.results.clear();
            s_motionLab.csvBuf.clear();
            s_motionLab.csvStatus[0] = '\0';
        }

        ImGui::Separator();

        // CSV
        ImGui::InputText("CSV Path", s_motionLab.csvPath, IM_ARRAYSIZE(s_motionLab.csvPath));
        if (ImGui::Button("Build CSV"))
        {
            MotionLab_RebuildCsvBuffer();
        }
        ImGui::SameLine();
        if (ImGui::Button("Copy CSV"))
        {
            if (s_motionLab.csvBuf.empty())
                MotionLab_RebuildCsvBuffer();
            ImGui::SetClipboardText(s_motionLab.csvBuf.data());
            s_motionLab.csvCopied = true;
            strcpy_s(s_motionLab.csvStatus, "Copied to clipboard.");
        }
        ImGui::SameLine();
        if (ImGui::Button("Save CSV"))
        {
            if (s_motionLab.csvBuf.empty())
                MotionLab_RebuildCsvBuffer();

            FILE* fp = nullptr;
            fopen_s(&fp, s_motionLab.csvPath, "wb");
            if (!fp)
            {
                strcpy_s(s_motionLab.csvStatus, "Save failed (fopen).");
            }
            else
            {
                // exclude null terminator
                const size_t n = (s_motionLab.csvBuf.size() > 0) ? (s_motionLab.csvBuf.size() - 1) : 0;
                fwrite(s_motionLab.csvBuf.data(), 1, n, fp);
                fclose(fp);
                s_motionLab.csvSaved = true;
                sprintf_s(s_motionLab.csvStatus, "Saved: %s", s_motionLab.csvPath);
            }
        }

        if (s_motionLab.csvStatus[0])
            ImGui::TextUnformatted(s_motionLab.csvStatus);

        if (!s_motionLab.csvBuf.empty())
        {
            ImGui::TextUnformatted("CSV Preview:");
            ImGui::InputTextMultiline("##csv_preview",
                s_motionLab.csvBuf.data(),
                s_motionLab.csvBuf.size(),
                ImVec2(-FLT_MIN, 160),
                ImGuiInputTextFlags_ReadOnly);
        }

        ImGui::Separator();

        const ImGuiTableFlags flags =
            ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersInnerV |
            ImGuiTableFlags_Resizable | ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_ScrollX;

        // 14 columns
        if (ImGui::BeginTable("##motion_results", 14, flags))
        {
            ImGui::TableSetupColumn("Name");
            ImGui::TableSetupColumn("t");
            ImGui::TableSetupColumn("distXZ");
            ImGui::TableSetupColumn("distAlong");
            ImGui::TableSetupColumn("targetDist");
            ImGui::TableSetupColumn("errDist");
            ImGui::TableSetupColumn("maxY");
            ImGui::TableSetupColumn("targetMaxY");
            ImGui::TableSetupColumn("errMaxY");
            ImGui::TableSetupColumn("hang");
            ImGui::TableSetupColumn("targetHang");
            ImGui::TableSetupColumn("errHang");
            ImGui::TableSetupColumn("vMaxXZ");
            ImGui::TableSetupColumn("vAvgXZ");
            ImGui::TableHeadersRow();

            for (auto& r : s_motionLab.results)
            {
                ImGui::TableNextRow();

                ImGui::TableSetColumnIndex(0); ImGui::TextUnformatted(r.name);
                ImGui::TableSetColumnIndex(1); ImGui::Text("%.3f", r.dt);

                ImGui::TableSetColumnIndex(2); ImGui::Text("%.3f", r.distXZ);
                ImGui::TableSetColumnIndex(3); ImGui::Text("%.3f", r.distAlong);

                ImGui::TableSetColumnIndex(4); ImGui::Text("%.3f", r.targetDist);
                ImGui::TableSetColumnIndex(5); ImGui::Text("%.3f", r.errDist);

                ImGui::TableSetColumnIndex(6); ImGui::Text("%.3f", r.maxY);
                ImGui::TableSetColumnIndex(7); ImGui::Text("%.3f", r.targetMaxY);
                ImGui::TableSetColumnIndex(8); ImGui::Text("%.3f", r.errMaxY);

                ImGui::TableSetColumnIndex(9);  ImGui::Text("%.3f", r.hangTime);
                ImGui::TableSetColumnIndex(10); ImGui::Text("%.3f", r.targetHang);
                ImGui::TableSetColumnIndex(11); ImGui::Text("%.3f", r.errHang);

                ImGui::TableSetColumnIndex(12); ImGui::Text("%.3f", r.vMaxXZ);
                ImGui::TableSetColumnIndex(13); ImGui::Text("%.3f", r.vAvgXZ);
            }

            ImGui::EndTable();
        }
    }

    ImGui::End();
}

void PlayerUI_Draw()
{
    if (!ImGui::Begin("Player Tuning"))
    {
        ImGui::End();
        return;
    }

    PlayerTuning* t = Player_GetTuning();
    const PlayerTuning& d = Player_GetDefaultTuning();

    static ImGuiTextFilter filter;
    filter.Draw("Filter");

    if (ImGui::Button("Reset All"))
        Player_ResetTuning();

    ImGui::Separator();

    struct Row
    {
        const char* name;
        float* v;
        float       def;
        float       speed;
        float       minv;
        float       maxv;
        const char* fmt;
        const char* help;
    };

    Row rows[] =
    {
        { "Jump Impulse", &t->jumpImpulse, d.jumpImpulse, 0.1f, 0.0f, 200.0f,  "%.3f", "Jump impulse applied on takeoff." },
        { "Gravity",      &t->gravity,     d.gravity,     0.1f, 0.0f, 200.0f,  "%.3f", "Downward acceleration (positive value)." },
        { "Move Accel",   &t->moveAccel,   d.moveAccel,   0.1f, 0.0f, 5000.0f, "%.3f", "Acceleration applied while moving." },
        { "Friction",     &t->friction,    d.friction,    0.1f, 0.0f, 100.0f,  "%.3f", "Velocity damping factor." },
        { "Rot Speed",    &t->rotSpeed,    d.rotSpeed,    0.1f, 0.0f, 50.0f,   "%.3f", "Rotation speed (rad/sec)." },
    };

    const ImGuiTableFlags flags =
        ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersInnerV |
        ImGuiTableFlags_Resizable | ImGuiTableFlags_SizingStretchProp;

    if (ImGui::BeginTable("##player_tuning", 4, flags))
    {
        ImGui::TableSetupColumn("Param");
        ImGui::TableSetupColumn("Value");
        ImGui::TableSetupColumn("Default", ImGuiTableColumnFlags_WidthFixed, 90.0f);
        ImGui::TableSetupColumn("Reset", ImGuiTableColumnFlags_WidthFixed, 60.0f);
        ImGui::TableHeadersRow();

        for (int i = 0; i < (int)IM_ARRAYSIZE(rows); ++i)
        {
            const Row& r = rows[i];
            if (!filter.PassFilter(r.name)) continue;

            ImGui::PushID(i);
            ImGui::TableNextRow();

            ImGui::TableSetColumnIndex(0);
            ImGui::TextUnformatted(r.name);
            if (r.help && ImGui::IsItemHovered())
                ImGui::SetTooltip("%s", r.help);

            ImGui::TableSetColumnIndex(1);
            ImGui::SetNextItemWidth(-FLT_MIN);
            ImGui::DragFloat("##v", r.v, r.speed, r.minv, r.maxv, r.fmt);

            ImGui::TableSetColumnIndex(2);
            ImGui::Text(r.fmt, r.def);

            ImGui::TableSetColumnIndex(3);
            if (ImGui::SmallButton("Reset"))
                *r.v = r.def;

            ImGui::PopID();
        }

        ImGui::EndTable();
    }

    ImGui::End();
}


bool DragFloat3Rebuild(const char* label, float* v, float speed)
{
    return ImGui::DragFloat3(label, v, speed);
}

static void GetFaceUvMinMax(const CubeFaceDesc& fd, float* outUvMin, float* outUvMax)
{
    outUvMin[0] = outUvMin[1] = +FLT_MAX;
    outUvMax[0] = outUvMax[1] = -FLT_MAX;
    for (int i = 0; i < CUBE_VERTS_PER_FACE; ++i)
    {
        const float u = fd.uv[i].x;
        const float v = fd.uv[i].y;
        outUvMin[0] = (u < outUvMin[0]) ? u : outUvMin[0];
        outUvMin[1] = (v < outUvMin[1]) ? v : outUvMin[1];
        outUvMax[0] = (u > outUvMax[0]) ? u : outUvMax[0];
        outUvMax[1] = (v > outUvMax[1]) ? v : outUvMax[1];
    }
}

static void KindEditor_Draw()
{
    if (!ImGui::Begin("Kind Editor"))
    {
        ImGui::End();
        return;
    }

    // Kind select
    ImGui::InputInt("Kind", &s_selectedKind);
    ImGui::SameLine();
    if (ImGui::Button("Use Selected Block Kind") && s_selected >= 0)
    {
        const StageBlock* b = Stage01_Get(s_selected);
        if (b) s_selectedKind = b->kind;
    }

    // Kind list (optional)
    int kinds[128];
    int total = Cube_GetKindList(kinds, 128);
    if (total > 0)
    {
        const int n = (total < 128) ? total : 128;
        std::sort(kinds, kinds + n);
        char cur[32];
        sprintf_s(cur, "%d", s_selectedKind);
        if (ImGui::BeginCombo("Kind List", cur))
        {
            for (int i = 0; i < n; ++i)
            {
                const int k = kinds[i];
                char label[32];
                sprintf_s(label, "%d", k);
                bool sel = (k == s_selectedKind);
                if (ImGui::Selectable(label, sel)) s_selectedKind = k;
                if (sel) ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }
    }

    CubeTemplate tpl{};
    bool hasTpl = Cube_TryGetKindTemplate(s_selectedKind, tpl);
    if (!hasTpl)
    {
        ImGui::TextUnformatted("This kind is not registered yet.");
        if (ImGui::Button("Create Kind (Unit Template)"))
        {
            tpl = CubeTemplate_Unit();
            Cube_RegisterKind(s_selectedKind, tpl);
            hasTpl = true;
        }
        ImGui::End();
        return;
    }

    ImGui::Separator();
    ImGui::Combo("Face", &s_selectedFace, kFaceNames, IM_ARRAYSIZE(kFaceNames));
    s_selectedFace = (s_selectedFace < 0) ? 0 : (s_selectedFace >= CUBE_FACE_COUNT ? (CUBE_FACE_COUNT - 1) : s_selectedFace);
    CubeFaceDesc& f = tpl.face[s_selectedFace];

    bool changed = false;

    // --- UV ---
    float uvMin[2], uvMax[2];
    GetFaceUvMinMax(f, uvMin, uvMax);
    bool uvChanged = false;
    uvChanged |= ImGui::DragFloat2("UV Min", uvMin, 0.01f);
    uvChanged |= ImGui::DragFloat2("UV Max", uvMax, 0.01f);

    if (uvChanged)
    {
        if (uvMin[0] > uvMax[0]) std::swap(uvMin[0], uvMax[0]);
        if (uvMin[1] > uvMax[1]) std::swap(uvMin[1], uvMax[1]);

        CubeTemplate_SetFaceUV(tpl, (CubeFace)s_selectedFace, { uvMin[0], uvMin[1] }, { uvMax[0], uvMax[1] });
        changed = true;
    }

    if (ImGui::Button("Apply UV to Face"))
    {
        CubeTemplate_SetFaceUV(tpl, (CubeFace)s_selectedFace, { uvMin[0], uvMin[1] }, { uvMax[0], uvMax[1] });
        changed = true;
    }
    ImGui::SameLine();
    if (ImGui::Button("Apply UV to ALL"))
    {
        CubeTemplate_SetAllFaceUV(tpl, { uvMin[0], uvMin[1] }, { uvMax[0], uvMax[1] });
        changed = true;
    }

    // --- Color ---
    float col[4] = { f.color[0].x, f.color[0].y, f.color[0].z, f.color[0].w };
    if (ImGui::ColorEdit4("Face Color", col))
    {
        CubeTemplate_SetFaceColor(tpl, (CubeFace)s_selectedFace, { col[0], col[1], col[2], col[3] });
        changed = true;
    }
    if (ImGui::Button("Apply Color to ALL"))
    {
        CubeTemplate_SetAllFaceColor(tpl, { col[0], col[1], col[2], col[3] });
        changed = true;
    }

    // --- Normal (optional) ---
    ImGui::Checkbox("Edit Normal", &s_editNormals);
    if (s_editNormals)
    {
        float n[3] = { f.normal.x, f.normal.y, f.normal.z };
        if (ImGui::DragFloat3("Normal", n, 0.01f, -1.0f, 1.0f))
        {
            f.normal = { n[0], n[1], n[2] };
            changed = true;
        }
    }

    if (changed)
        Cube_UpdateKind(s_selectedKind, tpl);

    ImGui::End();
}

void EditorUI_Draw(double elapsedTime)
{
    if (!ImGui::Begin("Stage Editor"))
    {
        ImGui::End();
        return;
    }

    if (!s_pathInited)
    {
        strcpy_s(s_jsonPath, Stage01_GetCurrentJsonPath());
        s_pathInited = true;
    }


    const int count = Stage01_GetCount();
    ImGui::Text("Blocks: %d", count);

    if (ImGui::Button("Add"))
    {
        StageBlock b{};
        b.kind = 0;
        b.texSlot = 0;
        b.texId = -1;
        b.position = { 0,0,0 };
        b.size = { 1,1,1 };
        b.rotation = { 0,0,0 };

        s_selected = Stage01_Add(b, true);
    }
    ImGui::SameLine();
    if (ImGui::Button("Rebuild All"))
    {
        Stage01_RebuildAll();
    }

    ImGui::Separator();

    // ===== 左：リスト =====
    ImGui::BeginChild("left_list", ImVec2(260, 0), true);
    for (int i = 0; i < count; ++i)
    {
        const StageBlock* b = Stage01_Get(i);
        if (!b) continue;

        char label[128];
        sprintf_s(label, "#%d  kind:%d  tex:%d", i, b->kind, b->texId);

        if (ImGui::Selectable(label, s_selected == i))
            s_selected = i;
    }
    ImGui::EndChild();

    ImGui::SameLine();

    // ===== 右：編集（スクロール可）=====
    ImGui::BeginChild("right_edit", ImVec2(0, 0), false, ImGuiWindowFlags_AlwaysVerticalScrollbar);

    if (ImGui::Button("Export C++ (Copy)"))
    {
        BuildStage01Cpp(s_exportText);
        ImGui::SetClipboardText(s_exportText.c_str());
        s_exportCopied = true;
    }
    if (s_exportCopied)
    {
        ImGui::SameLine();
        ImGui::TextUnformatted("Copied!");
    }

    // --- JSON Save/Load ---
    ImGui::Separator();
    ImGui::InputText("JSON Path", s_jsonPath, IM_ARRAYSIZE(s_jsonPath));

    if (ImGui::Button("Save JSON"))
    {
        const bool ok = Stage01_SaveJson(s_jsonPath);
        sprintf_s(s_ioStatus, ok ? "Saved: %s" : "Save failed: %s", s_jsonPath);
    }
    ImGui::SameLine();
    if (ImGui::Button("Load JSON"))
    {
        const bool ok = Stage01_LoadJson(s_jsonPath);
        if (ok) s_selected = (Stage01_GetCount() > 0) ? 0 : -1;
        sprintf_s(s_ioStatus, ok ? "Loaded: %s" : "Load failed: %s", s_jsonPath);
    }

    if (s_ioStatus[0])
        ImGui::TextUnformatted(s_ioStatus);
//-----------

    ImGui::Separator();

    StageBlock* b = Stage01_GetMutable(s_selected);
    if (!b)
    {
        ImGui::TextUnformatted("Select a block");
    }
    else
    {
        bool changed = false;

        changed |= ImGui::InputInt("Kind", &b->kind);

        int slot = b->texSlot;
        const int slotCount = Stage01_GetTexSlotCount();
        if (ImGui::BeginCombo("Texture", Stage01_GetTexSlotName(slot)))
        {
            for (int i = 0; i < slotCount; ++i)
            {
                bool selected = (i == slot);
                if (ImGui::Selectable(Stage01_GetTexSlotName(i), selected))
                {
                    b->texSlot = i;
                    changed = true;
                }
                if (selected) ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }

        changed |= DragFloat3Rebuild("Position", &b->position.x, 0.1f);
        changed |= DragFloat3Rebuild("Size", &b->size.x, 0.1f);
        changed |= DragFloat3Rebuild("Rotation", &b->rotation.x, 0.05f);

        ImGui::DragFloat("Snap", &s_snap, 0.1f, 0.0f, 10.0f);

        if (ImGui::Button("Snap Position"))
        {
            auto snapf = [&](float& v) {
                if (s_snap <= 0.0f) return;
                v = roundf(v / s_snap) * s_snap;
                };
            snapf(b->position.x); snapf(b->position.y); snapf(b->position.z);
            changed = true;
        }

        // ===== Duplicate =====
        ImGui::Separator();
        ImGui::TextUnformatted("Duplicate");
        ImGui::DragFloat3("Dup Offset", s_dupOffset, 0.1f);
        ImGui::Checkbox("Offset * Snap", &s_dupOffsetUseSnap);
        ImGui::InputInt("Dup Count", &s_dupCount);
        if (s_dupCount < 1)   s_dupCount = 1;
        s_dupCount = (s_dupCount > 256) ? 256 : s_dupCount;


        if (ImGui::Button("Duplicate Selected"))
        {
            const StageBlock* srcPtr = Stage01_Get(s_selected);
            if (srcPtr)
            {
                const StageBlock srcBlock = *srcPtr;

                const float mul = s_dupOffsetUseSnap ? s_snap : 1.0f;
                const DirectX::XMFLOAT3 step{
                    s_dupOffset[0] * mul,
                    s_dupOffset[1] * mul,
                    s_dupOffset[2] * mul
                };

                int newIndex = -1;
                for (int n = 0; n < s_dupCount; ++n)
                {
                    StageBlock nb = srcBlock;
                    const float t = float(n + 1);
                    nb.position.x += step.x * t;
                    nb.position.y += step.y * t;
                    nb.position.z += step.z * t;

                    newIndex = Stage01_Add(nb, true); // ApplyTex + Bake
                }

                if (newIndex >= 0) s_selected = newIndex;
            }
        }


        if (changed)
            Stage01_RebuildObject(s_selected);

        if (ImGui::Button("Delete"))
        {
            Stage01_Remove(s_selected);
            s_selected = (Stage01_GetCount() > 0) ? std::min(s_selected, Stage01_GetCount() - 1) : -1;
        }
    }

    ImGui::EndChild(); // right_edit

    ImGui::End(); // Stage Editor

    KindEditor_Draw();

    // プレイヤー調整は別ウィンドウ
    PlayerUI_Draw();

    MotionLab_Draw(elapsedTime);
}

void BuildStage01Cpp(std::string& out)
{
    std::ostringstream oss;
    oss << "static const StageRow kStage01[] =\n{\n";
    oss << std::fixed << std::setprecision(3);

    const int n = Stage01_GetCount();
    for (int i = 0; i < n; ++i)
    {
        const StageBlock* b = Stage01_Get(i);
        if (!b) continue;

        oss << "    { "
            << b->kind << ", " << b->texSlot << ", "
            << b->position.x << "f, " << b->position.y << "f, " << b->position.z << "f, "
            << b->size.x << "f, " << b->size.y << "f, " << b->size.z << "f, "
            << b->rotation.x << "f, " << b->rotation.y << "f, " << b->rotation.z << "f"
            << " },\n";
    }

    oss << "};\n\n"
        << "void Stage01_MakeTable(const StageRow** outRows, int* outCount)\n"
        << "{\n"
        << "    *outRows = kStage01;\n"
        << "    *outCount = (int)(sizeof(kStage01) / sizeof(kStage01[0]));\n"
        << "}\n";

    out = oss.str();
}
