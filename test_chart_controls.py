import argparse
from pathlib import Path
import subprocess
import shutil
import struct

from scripts import em_probe as probe
import tempfile

ROOT = Path(__file__).resolve().parent

HARNESS = r'''
using System;
using System.Reflection;
using System.Windows.Forms;
using System.Drawing;
using System.Windows.Forms.DataVisualization.Charting;

public class ChartControlsTest {
    const BindingFlags Flags = BindingFlags.Instance | BindingFlags.NonPublic | BindingFlags.Public;
    static object Get(Form form, string name) { return form.GetType().GetField(name, Flags).GetValue(form); }
    static void Set(Form form, string name, object value) { form.GetType().GetField(name, Flags).SetValue(form, value); }
    static object Call(Form form, string name, params object[] args) { return form.GetType().GetMethod(name, Flags).Invoke(form, args); }
    static Control Control(Form form, string name) { return (Control)Get(form, name); }
    static void Check(bool value, string message) { if (!value) throw new Exception(message); }
    static void Click(Form form, string name) { ((Button)Get(form, name)).PerformClick(); }

    static void SelectCurve(Form form, int window, string curve) {
        using (var item = new ToolStripMenuItem(curve)) {
            item.Tag = window;
            Call(form, "CurveMenuItem_Click", item, EventArgs.Empty);
        }
    }
    static void Direct(Form form, int window, bool direct) {
        if ((bool)Get(form, "isDirectMode" + window) != direct) Click(form, "buttonMode" + window);
    }
    static void ZoomY(Chart chart) {
        chart.ChartAreas[0].RecalculateAxesScale();
        var axis = chart.ChartAreas[0].AxisY;
        axis.ScaleView.ZoomReset(0);
        double minimum = axis.ScaleView.ViewMinimum, maximum = axis.ScaleView.ViewMaximum;
        axis.ScaleView.Zoom(minimum + (maximum - minimum) * 0.2, minimum + (maximum - minimum) * 0.4);
        Check(axis.ScaleView.IsZoomed, "Y zoom setup failed");
    }
    static void AutoY(Chart chart) {
        Application.DoEvents();
        var axis = chart.ChartAreas[0].AxisY;
        Check(!axis.ScaleView.IsZoomed, "Old Y zoom survived data switch");
        foreach (Series series in chart.Series) {
            if (!series.Enabled) continue;
            foreach (DataPoint point in series.Points) {
                if (point.IsEmpty) continue;
                Check(point.YValues[0] >= axis.ScaleView.ViewMinimum - 0.001 && point.YValues[0] <= axis.ScaleView.ViewMaximum + 0.001,
                    "Auto Y range does not contain new data");
            }
        }
    }
    static void Scheme(Form form, int chart, bool direct) {
        object scheme = Get(form, "sondeScheme");
        Check((int)Get(form, "activeSchemeChart") == chart, "Scheme bound to wrong chart");
        Check((int)scheme.GetType().GetField("chartIndex", Flags).GetValue(scheme) == chart, "Scheme badge bound to wrong chart");
        Check((bool)scheme.GetType().GetField("isDirect", Flags).GetValue(scheme) == direct, "Scheme mode not updated");
    }
    static void TestSwitching(Form form, string dataFile) {
        Call(form, "LoadDataFile", System.IO.Path.Combine(System.IO.Path.GetDirectoryName(dataFile), "scales.bin"));
        Chart first = (Chart)Get(form, "chartTime1"), second = (Chart)Get(form, "chartTime2");
        ((CheckBox)Get(form, "checkHoldZoom1")).Checked = false;
        ((CheckBox)Get(form, "checkHoldZoom2")).Checked = false;
        Direct(form, 1, true);
        Direct(form, 2, true);
        for (int window = 1; window <= 2; ++window) {
            Chart chart = window == 1 ? first : second;
            Chart other = window == 1 ? second : first;
            SelectCurve(form, window, "AM VZX");
            ZoomY(chart);
            ZoomY(other);
            double otherPosition = other.ChartAreas[0].AxisY.ScaleView.Position;
            Click(form, "buttonFreq" + window);
            AutoY(chart);
            Check(other.ChartAreas[0].AxisY.ScaleView.IsZoomed && other.ChartAreas[0].AxisY.ScaleView.Position == otherPosition,
                "Switch changed the other chart zoom");
            Scheme(form, window - 1, true);
            ZoomY(chart);
            Click(form, "buttonReceiver" + window);
            AutoY(chart);
            Scheme(form, window - 1, true);
            ZoomY(chart);
            SelectCurve(form, window, window == 1 ? "SNR" : "dV");
            AutoY(chart);
            ZoomY(chart);
            foreach (Control chip in Control(form, "flowPanelChips" + window).Controls) {
                if ((string)chip.Tag != "AM VZX") continue;
                Call(form, "Chip_Click", chip, EventArgs.Empty);
                break;
            }
            AutoY(chart);
            SelectCurve(form, window, window == 1 ? "SNR" : "dV");
            ZoomY(chart);
            Direct(form, window, false);
            AutoY(chart);
            Scheme(form, window - 1, false);
            SelectCurve(form, window, window == 1 ? "AMP Z2" : "RO ATT Sonde");
            Direct(form, window, true);
            Check((string)Get(form, "activeCurve" + window) == (window == 1 ? "SNR" : "dV"), "DIRECT curve not restored");
            Scheme(form, window - 1, true);
            for (int i = 0; i < 2; ++i) {
                Click(form, "buttonFreq" + window);
                AutoY(chart);
            }
            if ((int)Get(form, "receiverIndex" + window) != 0) Click(form, "buttonReceiver" + window);
            Check(chart.Series[0].Color == Color.Green && chart.Series[1].Color == Color.DarkGoldenrod, "Left length colors wrong");
            Check(chart.Series[2].LegendText == "T4" && chart.Series[3].LegendText == "T5", "Left Tx labels wrong");
            Click(form, "buttonReceiver" + window);
            Check(chart.Series[0].Color == Color.DarkGoldenrod && chart.Series[1].Color == Color.Green, "Right length colors not mirrored");
            Check(chart.Series[2].Color == Color.Red && chart.Series[3].Color == Color.RoyalBlue, "Long length colors changed");
            Check(chart.Series[2].LegendText == "T3", "Right Tx labels wrong");
            Check(Control(form, "checkTx" + window + "_1").ForeColor == chart.Series[0].Color, "Tx checkbox color differs from curve");
        }
        Call(form, "SaveStationConfig");
        Call(form, "LoadStationConfig");
        for (int window = 1; window <= 2; ++window) {
            Direct(form, window, false);
            Check((string)Get(form, "activeCurve" + window) == (window == 1 ? "AMP Z2" : "RO ATT Sonde"), "UNDIRECT curve not restored from config");
            Direct(form, window, true);
            Check((string)Get(form, "activeCurve" + window) == (window == 1 ? "SNR" : "dV"), "DIRECT curve not restored from config");
            Check(((System.Collections.IList)Get(form, "pinnedCurves" + window)).Contains("AM VZX"), "DIRECT pinned list lost");
        }
        ((CheckBox)Get(form, "checkHoldZoom1")).Checked = true;
        ZoomY(first);
        first.ChartAreas[0].AxisX.ScaleView.Zoom(3, 25);
        double heldY = first.ChartAreas[0].AxisY.ScaleView.ViewMinimum;
        double heldX = first.ChartAreas[0].AxisX.ScaleView.ViewMinimum;
        Click(form, "buttonFreq1");
        Check(first.ChartAreas[0].AxisY.ScaleView.IsZoomed && first.ChartAreas[0].AxisY.ScaleView.ViewMinimum == heldY, "Hold Y ignored");
        Check(first.ChartAreas[0].AxisX.ScaleView.IsZoomed && first.ChartAreas[0].AxisX.ScaleView.ViewMinimum == heldX, "Hold X ignored");
        ((CheckBox)Get(form, "checkHoldZoom1")).Checked = false;
        Call(form, "StartDisplay");
        for (int window = 1; window <= 2; ++window) {
            Chart chart = window == 1 ? first : second;
            double minimum = chart.ChartAreas[0].AxisX.Minimum, maximum = chart.ChartAreas[0].AxisX.Maximum;
            ZoomY(chart);
            Click(form, "buttonFreq" + window);
            AutoY(chart);
            Check(chart.ChartAreas[0].AxisX.Minimum == minimum && chart.ChartAreas[0].AxisX.Maximum == maximum, "Stream frame window lost on frequency switch");
            ZoomY(chart);
            Click(form, "buttonReceiver" + window);
            AutoY(chart);
            Check(chart.ChartAreas[0].AxisX.Minimum == minimum && chart.ChartAreas[0].AxisX.Maximum == maximum, "Stream frame window lost on receiver switch");
            Direct(form, window, false);
            AutoY(chart);
            Direct(form, window, true);
            AutoY(chart);
        }
        Call(form, "StopDisplay");
        string config = System.IO.Path.Combine(Application.StartupPath, "station_config.json");
        var legacy = new System.Collections.Generic.List<string>();
        foreach (string line in System.IO.File.ReadAllLines(config)) {
            if (line.Contains("DirectActiveCurve") || line.Contains("UndirectActiveCurve") ||
                line.Contains("DirectPinnedCurves") || line.Contains("UndirectPinnedCurves")) continue;
            legacy.Add(line);
        }
        System.IO.File.WriteAllLines(config, legacy.ToArray());
        Call(form, "LoadStationConfig");
        for (int window = 1; window <= 2; ++window) {
            string active = (string)Get(form, "activeCurve" + window);
            Direct(form, window, false);
            Direct(form, window, true);
            Check((string)Get(form, "activeCurve" + window) == active, "Legacy config did not seed mode memory");
        }
        Console.WriteLine("Chart switching: both windows, mode memory, scheme, length colors, frequency/LR/curve autoscale, hold zoom, stream window PASS");
    }

    static void CloseChip(Form form, Control chip) {
        foreach (Control child in chip.Controls)
            if (child is Button) { Call(form, "ChipCloseButton_Click", child, EventArgs.Empty); return; }
        throw new Exception("Chip close button not found");
    }
    static void TestChipClose(Form form, string dataFile) {
        Call(form, "LoadDataFile", dataFile);
        Call(form, "ApplySondeTypeLayout", true);
        Chart chart = (Chart)Get(form, "chartTime2");
        Direct(form, 2, false);
        SelectCurve(form, 2, "AM Z2/Z1");
        SelectCurve(form, 2, "Raw Δφ");
        SelectCurve(form, 2, "AMP Z1");
        foreach (Control chip in Control(form, "flowPanelChips2").Controls)
            if ((string)chip.Tag == "AMP Z1") { CloseChip(form, chip); break; }
        Check((string)Get(form, "activeCurve2") == "Raw Δφ", "Closing active curve did not select the previous one");
        var remaining = new System.Collections.Generic.List<Control>();
        foreach (Control chip in Control(form, "flowPanelChips2").Controls) remaining.Add(chip);
        foreach (Control chip in remaining) CloseChip(form, chip);
        Check(string.IsNullOrEmpty((string)Get(form, "activeCurve2")), "Active curve survived closing all chips");
        foreach (Series series in chart.Series)
            Check(series.Points.Count == 0, "Closed curves still rendered on chart");
        SelectCurve(form, 2, "AM Z2/Z1");
        Check(chart.Series[0].Points.Count > 0, "Chart did not update after adding a curve");
        var modePinned = (System.Collections.Generic.List<string>[])Get(form, "modePinnedCurves");
        var modeActive = (string[])Get(form, "modeActiveCurves");
        modePinned[3] = new System.Collections.Generic.List<string> { "AM Z2/Z1", "AM GEO" };
        modeActive[3] = "AM Z2/Z1";
        Direct(form, 2, true);
        Check(!((System.Collections.IList)Get(form, "pinnedCurves2")).Contains("AM Z2/Z1"), "Foreign-mode pinned curve restored into DIRECT");
        Check((string)Get(form, "activeCurve2") == "AM GEO", "Foreign-mode active curve restored into DIRECT");
        string config = System.IO.Path.Combine(Application.StartupPath, "station_config.json");
        System.IO.File.WriteAllText(config,
            "{\n  \"Window2\": {\n    \"DirectActiveCurve\": \"AM Z2/Z1\",\n" +
            "    \"DirectPinnedCurves\": [\"AM Z2/Z1\", \"RO ATT Sonde\"],\n" +
            "    \"ActiveCurve\": \"AM Z2/Z1\",\n    \"Mode\": \"DIRECT\",\n" +
            "    \"PinnedCurves\": [\"AM Z2/Z1\"]\n  },\n  \"Sectors\": {}\n}\n");
        Call(form, "LoadStationConfig");
        Check(!((System.Collections.IList)Get(form, "pinnedCurves2")).Contains("AM Z2/Z1"), "Dirty config put an UNDIRECT curve on a DIRECT chart");
        Check((string)Get(form, "activeCurve2") != "AM Z2/Z1", "Dirty config activated an UNDIRECT curve on a DIRECT chart");
        Check(modePinned[3].Count == 0 && modeActive[3] == null, "Dirty mode history was not filtered on load");
        System.IO.File.WriteAllText(config,
            "{\n  \"Window1\": {\n    \"Mode\": \"UNDIRECT\",\n    \"PinnedCurves\": []\n  },\n" +
            "  \"Window2\": {\n    \"Mode\": \"UNDIRECT\",\n    \"PinnedCurves\": []\n  },\n  \"Sectors\": {}\n}\n");
        Call(form, "LoadStationConfig");
        Check(((System.Collections.IList)Get(form, "pinnedCurves1")).Count == 0 &&
            ((System.Collections.IList)Get(form, "pinnedCurves2")).Count == 0,
            "Explicitly saved empty curve list was replaced by defaults");
        Console.WriteLine("Chip close: empty chart, previous curve activation, dirty config filtering PASS");
    }

    public static int Run(Form form, string dataFile) {
        try {
            form.ShowInTaskbar = false;
            form.StartPosition = FormStartPosition.Manual;
            form.Location = new Point(-32000, -32000);
            form.Show();
            Application.DoEvents();
            Call(form, "LoadDataFile", dataFile);
            Call(form, "ApplySondeTypeLayout", true);
            Check(Control(form, "buttonMode1").Visible && Control(form, "buttonMode2").Visible, "Cartograph mode buttons hidden");
            Check(!Control(form, "buttonReceiver1").Visible && !Control(form, "buttonReceiver2").Visible, "Receiver visible in UNDIRECT");
            Click(form, "buttonMode1");
            Check(Control(form, "buttonReceiver1").Visible && !Control(form, "buttonReceiver2").Visible, "DIRECT receiver visibility incorrect");
            Set(form, "activeCurve1", "AM VZX");
            Call(form, "RenderLoadedCurves");
            Chart chart1 = (Chart)Get(form, "chartTime1");
            Chart chart2 = (Chart)Get(form, "chartTime2");
            Check(chart1.Series[0].Points[0].YValues[0] == 100.0, "Left receiver data not rendered");
            Click(form, "buttonReceiver1");
            Check(chart1.Series[0].Points[0].YValues[0] == 1000.0, "Right receiver data not rendered");
            Check((int)Get(form, "receiverIndex1") == 1 && (int)Get(form, "receiverIndex2") == 0, "Receiver choices not independent");
            Click(form, "buttonMode2");
            Check(Control(form, "buttonReceiver2").Visible, "Second receiver button hidden in DIRECT");
            Set(form, "activeCurve2", "AM VZX");
            Call(form, "RenderLoadedCurves");
            Check(chart2.Series[0].Points[0].YValues[0] == 100.0, "Second chart did not keep left receiver");
            Call(form, "SaveStationConfig");
            Set(form, "receiverIndex1", 0);
            Call(form, "LoadStationConfig");
            Check((int)Get(form, "receiverIndex1") == 1 && (int)Get(form, "receiverIndex2") == 0, "Receiver config round trip failed");
            Check(Control(form, "buttonReceiver1").Text == "Rx R", "Receiver label not restored");
            Click(form, "buttonXMode1");
            Click(form, "buttonXMode2");
            Check((bool)Get(form, "isTempXAxis1") && (bool)Get(form, "isTempXAxis2"), "Temperature axis not selected");
            Call(form, "StartDisplay");
            Check(!(bool)Get(form, "isTempXAxis1") && !(bool)Get(form, "isTempXAxis2"), "Stream retained temperature axis");
            Check(!Control(form, "buttonXMode1").Visible && !Control(form, "buttonXMode2").Visible, "Axis buttons visible in stream");
            Call(form, "buttonXMode1_Click", Control(form, "buttonXMode1"), EventArgs.Empty);
            Check(!(bool)Get(form, "isTempXAxis1"), "Hidden axis handler changed stream axis");
            Click(form, "buttonReceiver2");
            Check((int)Get(form, "receiverIndex2") == 1, "Receiver cannot change while streaming");
            Call(form, "AppendFrameToCharts", (UInt64)0);
            Check(chart1.Series[0].Points[chart1.Series[0].Points.Count - 1].YValues[0] == 1000.0, "Stream did not use first chart receiver");
            Check(chart2.Series[0].Points[chart2.Series[0].Points.Count - 1].YValues[0] == 1000.0, "Stream did not use second chart receiver");
            Check(chart1.Series[0].Points[0].XValue == 0.0 && chart2.Series[0].Points[0].XValue == 0.0, "Stream X is not frame index");
            Call(form, "StopDisplay");
            Check(Control(form, "buttonXMode1").Visible && Control(form, "buttonXMode2").Visible, "Axis buttons not restored");
            Call(form, "LoadDataFile", System.IO.Path.Combine(System.IO.Path.GetDirectoryName(dataFile), "gp.bin"));
            Check(!Control(form, "buttonMode1").Visible && !Control(form, "buttonMode2").Visible, "Mode buttons visible for GP");
            Check(!Control(form, "buttonReceiver1").Visible && !Control(form, "buttonReceiver2").Visible, "Receiver buttons visible for GP");
            Check(!(bool)Get(form, "isDirectMode1") && !(bool)Get(form, "isDirectMode2"), "GP retained DIRECT mode");
            Call(form, "buttonMode1_Click", Control(form, "buttonMode1"), EventArgs.Empty);
            Check(!(bool)Get(form, "isDirectMode1"), "Hidden mode handler enabled DIRECT for GP");
            Call(form, "LoadDataFile", dataFile);
            Check(Control(form, "buttonMode1").Visible && !Control(form, "buttonReceiver1").Visible, "Cartograph visibility not restored");
            Console.WriteLine("Chart controls: mode visibility, independent L/R, config round trip, stream axes PASS");
            TestSwitching(form, dataFile);
            TestChipClose(form, dataFile);
            return 0;
        } catch (Exception error) {
            Console.Error.WriteLine(error);
            return 1;
        } finally {
            if (form != null) { form.Close(); form.Dispose(); }
        }
    }
}
'''


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--config", choices=("Debug", "Release"), default="Debug")
    args = parser.parse_args()
    output = ROOT / "x64" / args.config
    objects = ROOT / "ELECTROMAGNETIC_TOOLS_3.0" / "x64" / args.config
    csc = Path(r"C:\Windows\Microsoft.NET\Framework64\v4.0.30319\csc.exe")
    devcmd = Path(r"C:\Program Files\Microsoft Visual Studio\18\Community\Common7\Tools\VsDevCmd.bat")
    references = Path(r"C:\Program Files (x86)\Reference Assemblies\Microsoft\Framework\.NETFramework\v4.8")
    with tempfile.TemporaryDirectory(prefix="em_chart_controls_", ignore_cleanup_errors=True) as directory:
        work = Path(directory)
        source = work / "test.cs"
        source.write_text(HARNESS, encoding="utf-8")
        command = [str(csc), "/nologo", "/target:library", "/platform:x64", "/r:System.Windows.Forms.dll", "/r:System.Drawing.dll", f"/r:{references / 'System.Windows.Forms.DataVisualization.dll'}", "/out:controls.dll", str(source)]
        subprocess.run(command, cwd=work, check=True)
        shutil.copy2(output / "EmUpdate.exe", work / "EmUpdate.exe")
        driver = '\n'.join(f'#using "{name}"' for name in ("System.dll", "System.Data.dll", "System.Drawing.dll", "System.Windows.Forms.dll", "System.Windows.Forms.DataVisualization.dll", "EmUpdate.exe", "controls.dll"))
        driver += '\n#include "MainForm.h"\n[System::STAThreadAttribute] int main(array<System::String^>^ args) { System::Windows::Forms::Application::EnableVisualStyles(); return ChartControlsTest::Run(gcnew ELECTROMAGNETIC_TOOLS_3_0::MainForm(), args[0]); }'
        (work / "driver.cpp").write_text(driver, encoding="utf-8")
        runtime = "/MDd /D_DEBUG" if args.config == "Debug" else "/MD /DNDEBUG"
        inputs = " ".join(f'"{objects / (name + ".obj")}"' for name in ("MainForm", "ConnectionForm", "MetroForm", "BinFileReader", "pch", "AssemblyInfo"))
        command = f'call "{devcmd}" -arch=x64 >nul && cl /nologo /clr /EHa /std:c++17 /utf-8 {runtime} /AI"{references}" /I"{ROOT / "ELECTROMAGNETIC_TOOLS_3.0"}" driver.cpp {inputs} /Fe:test.exe'
        build = subprocess.run(command, cwd=work, shell=True, capture_output=True)
        if build.returncode:
            print(build.stdout.decode(errors="replace"))
            print(build.stderr.decode(errors="replace"))
            raise SystemExit(build.returncode)
        (work / "test.exe.config").write_text('<configuration><startup useLegacyV2RuntimeActivationPolicy="true"><supportedRuntime version="v4.0" /></startup></configuration>', encoding="utf-8")
        data = bytearray(probe.pack_all_data(1536351001, 1, 1, 25, 15, 0.3, 0, 45, 90, 2, 1))
        for frequency in range(2):
            struct.pack_into("<I", data, frequency * 512 + 8, (401, 2001)[frequency])
            for receiver in range(2):
                for tx in range(4):
                    struct.pack_into("<f", data, frequency * 512 + 116 + receiver * 128 + 56 + tx * 4, 100 + receiver * 900)
        fixture = work / "cartograph.bin"
        fixture.write_bytes(data)
        (work / "gp.bin").write_bytes(probe.pack_gp_data(320349001, 1, 25, 4, 0.3))
        scales = bytearray()
        for frame in range(40):
            record = bytearray(data)
            for frequency in range(2):
                struct.pack_into("<I", record, frequency * 512 + 4, frame + 1)
                factor = 1000 if frequency else 1
                for receiver in range(2):
                    for tx in range(4):
                        amplitude = (100 + receiver * 900 + frame + tx) * factor
                        struct.pack_into("<f", record, frequency * 512 + 116 + receiver * 128 + 56 + tx * 4, amplitude)
            scales.extend(record)
        (work / "scales.bin").write_bytes(scales)
        subprocess.run([str(work / "test.exe"), str(fixture)], cwd=work, check=True, timeout=30)


if __name__ == "__main__":
    main()
