using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.IO.Compression;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace EmTools.Updates
{
    public sealed class RuntimeJournal
    {
        public string component = PythonRuntime.Component;
        public string target;
        public string incoming;
        public bool had_old;
        public string status;
    }

    public static class PythonRuntime
    {
        public const string Component = "python-runtime";
        public const string DirectoryName = "python_runtime";
        public const string State = "runtime-release.json";
        public const int ManifestLimit = 16 * 1024 * 1024;
        public static readonly string[] Packages = { "numpy", "scipy", "pandas", "matplotlib", "tcl" };
        const string ProbeCode = @"import sys, os, struct, json, io
import numpy, scipy, pandas, matplotlib, tkinter
matplotlib.use('Agg')
from matplotlib import pyplot
base = os.path.normcase(os.path.realpath(os.path.dirname(sys.executable)))
modules = (numpy, scipy, pandas, matplotlib, tkinter)
portable = all(os.path.commonpath([base, os.path.normcase(os.path.realpath(m.__file__))]) == base for m in modules)
assert numpy.isclose(numpy.dot([1,2],[3,4]),11)
from scipy import linalg
assert abs(linalg.det([[1,0],[0,2]])-2)<1e-8
assert pandas.DataFrame({'x':[1,2]}).x.sum()==3
figure=pyplot.figure(); pyplot.plot([0,1],[0,1]); figure.savefig(io.BytesIO(),format='png'); pyplot.close(figure)
window=tkinter.Tk(); window.withdraw(); tcl=window.tk.eval('info patchlevel'); window.destroy()
print(json.dumps(dict(python=sys.version.split()[0],bits=struct.calcsize('P')*8,portable=portable,numpy=numpy.__version__,scipy=scipy.__version__,pandas=pandas.__version__,matplotlib=matplotlib.__version__,tcl=tcl)))";

        internal static bool ValidPath(string path)
        {
            if (String.IsNullOrEmpty(path) || path.Length > 220 || path.Contains('\\') || Path.IsPathRooted(path)
                || String.Equals(path, State, StringComparison.OrdinalIgnoreCase) || String.Equals(path, "manifest.json", StringComparison.OrdinalIgnoreCase)) return false;
            foreach (string part in path.Split('/'))
            {
                if (part.Length == 0 || part == "." || part == ".." || part.EndsWith(".") || part.EndsWith(" ") || part.IndexOfAny(Path.GetInvalidFileNameChars()) >= 0) return false;
                string stem = part.Split('.')[0].ToUpperInvariant();
                if (new[] { "CON", "PRN", "AUX", "NUL", "COM1", "COM2", "COM3", "COM4", "COM5", "COM6", "COM7", "COM8", "COM9", "LPT1", "LPT2", "LPT3", "LPT4", "LPT5", "LPT6", "LPT7", "LPT8", "LPT9" }.Contains(stem)) return false;
            }
            return true;
        }

        public static Manifest ReadManifest(string path)
        {
            UpdateEngine.Require(new FileInfo(path).Length <= ManifestLimit, "Манифест Python слишком велик.");
            Manifest m = UpdateEngine.Json.Deserialize<Manifest>(File.ReadAllText(path, Encoding.UTF8));
            UpdateEngine.Require(m != null && m.schema == 1 && m.product == UpdateEngine.Product && m.platform == "x64"
                && m.component == Component && m.authentication == "none", "Неподдерживаемый пакет Python Runtime.");
            Version version;
            DateTime released;
            UpdateEngine.Require(m.build > 0 && Version.TryParse(m.version, out version) && DateTime.TryParse(m.released_utc,
                System.Globalization.CultureInfo.InvariantCulture, System.Globalization.DateTimeStyles.RoundtripKind, out released), "Некорректная версия Python Runtime.");
            UpdateEngine.Require(m.notes != null && m.notes.Length <= 16000 && m.package != null && m.package.StartsWith("python-runtime-", StringComparison.Ordinal)
                && m.package.EndsWith(".zip", StringComparison.Ordinal) && m.package == Path.GetFileName(m.package)
                && m.package.All(c => c < 128 && Char.IsLetterOrDigit(c) || c == '-' || c == '.'), "Некорректное имя архива Python.");
            UpdateEngine.Require(m.size > 0 && m.size <= UpdateEngine.MaxPackage && UpdateEngine.ValidHash(m.sha256), "Некорректный размер/SHA-256 Runtime.");
            UpdateEngine.Require(m.packages != null && m.packages.Count == Packages.Length && Packages.All(p => m.packages.ContainsKey(p)
                && !String.IsNullOrEmpty(m.packages[p]) && m.packages[p].Length <= 64), "Не указан состав библиотек Python.");
            UpdateEngine.Require(m.files != null && m.files.Length > 0 && m.files.Length <= 20000, "Некорректный состав Runtime.");
            var names = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
            long total = 0;
            foreach (PayloadFile file in m.files)
            {
                UpdateEngine.Require(file != null && ValidPath(file.path) && names.Add(file.path), "Недопустимый/повторный путь Python Runtime.");
                UpdateEngine.Require(file.size >= 0 && file.size <= UpdateEngine.MaxPackage && UpdateEngine.ValidHash(file.sha256), "Некорректный файл Python Runtime.");
                total += file.size;
            }
            foreach (string name in names)
            {
                string parent = name;
                while (parent.Contains('/'))
                {
                    parent = parent.Substring(0, parent.LastIndexOf('/'));
                    UpdateEngine.Require(!names.Contains(parent), "Файл конфликтует с каталогом Runtime.");
                }
            }
            UpdateEngine.Require(total <= 1024L * 1024 * 1024 && names.Contains("python.exe") && names.Any(n => n.IndexOf('/') < 0 && n.EndsWith("._pth", StringComparison.Ordinal)), "Требуется portable Python x64 с ._pth; превышен размер Runtime.");
            return m;
        }

        public static long CurrentBuild(string target)
        {
            string path = Path.Combine(target, DirectoryName, State);
            return File.Exists(path) ? ReadManifest(path).build : 0;
        }

        public static string Prepare(string manifestPath, string zipPath, string target)
        {
            UpdateEngine.NoLinks(target);
            UpdateEngine.Require(Directory.Exists(target) && UpdateEngine.PendingBackup(target) == null, "Папка недоступна или есть незавершенная установка.");
            Manifest m = ReadManifest(manifestPath);
            UpdateEngine.Require(m.build > CurrentBuild(target), "Установлена такая же или более новая среда Python.");
            UpdateEngine.VerifyFile(zipPath, m.size, m.sha256);
            string stage = Path.Combine(Path.GetTempPath(), "EmToolsPython-" + Guid.NewGuid().ToString("N"));
            Directory.CreateDirectory(stage);
            try
            {
                var expected = m.files.ToDictionary(f => f.path, StringComparer.Ordinal);
                using (var zip = ZipFile.OpenRead(zipPath))
                {
                    UpdateEngine.Require(zip.Entries.Count == expected.Count, "Состав ZIP Runtime не совпадает с манифестом.");
                    var seen = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
                    foreach (var entry in zip.Entries)
                    {
                        PayloadFile file;
                        UpdateEngine.Require(expected.TryGetValue(entry.FullName, out file) && seen.Add(entry.FullName)
                            && entry.Length == file.size, "Лишний/повторный файл в ZIP Runtime.");
                        UpdateEngine.Require((entry.ExternalAttributes & 0x400) == 0 && ((entry.ExternalAttributes >> 16) & 0xF000) != 0xA000, "Ссылки в Runtime запрещены.");
                        string destination = Path.Combine(stage, file.path.Replace('/', Path.DirectorySeparatorChar));
                        Directory.CreateDirectory(Path.GetDirectoryName(destination));
                        using (var input = entry.Open())
                        using (var output = new FileStream(destination, FileMode.CreateNew))
                        {
                            byte[] buffer = new byte[65536];
                            long length = 0;
                            int read;
                            while ((read = input.Read(buffer, 0, buffer.Length)) != 0)
                            {
                                length += read;
                                UpdateEngine.Require(length <= file.size, "Превышен размер файла Runtime.");
                                output.Write(buffer, 0, read);
                            }
                            UpdateEngine.Require(length == file.size, "Неполный файл Runtime.");
                        }
                        UpdateEngine.VerifyFile(destination, file.size, file.sha256);
                    }
                }
                UpdateEngine.WriteJson(Path.Combine(stage, "manifest.json"), m);
                return stage;
            }
            catch { Directory.Delete(stage, true); throw; }
        }

        internal static void CleanupStage(string stage, Manifest manifest)
        {
            if (!UpdateEngine.Same(Path.GetDirectoryName(stage), Path.GetTempPath()) || !Path.GetFileName(stage).StartsWith("EmToolsPython-", StringComparison.Ordinal)) return;
            var directories = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
            foreach (PayloadFile file in manifest.files)
            {
                string relative = file.path.Replace('/', Path.DirectorySeparatorChar);
                string path = Path.Combine(stage, relative);
                UpdateEngine.NoLinks(path);
                if (File.Exists(path)) File.Delete(path);
                string parent = Path.GetDirectoryName(relative);
                while (!String.IsNullOrEmpty(parent)) { directories.Add(parent); parent = Path.GetDirectoryName(parent); }
            }
            foreach (string directory in directories.OrderByDescending(d => d.Length))
            {
                string path = Path.Combine(stage, directory);
                UpdateEngine.NoLinks(path);
                if (Directory.Exists(path)) Directory.Delete(path, false);
            }
            File.Delete(Path.Combine(stage, "manifest.json"));
            Directory.Delete(stage, false);
        }

        internal static string Quote(string value)
        {
            var result = new StringBuilder("\"");
            int slashes = 0;
            foreach (char c in value)
            {
                if (c == '\\') { ++slashes; continue; }
                result.Append('\\', c == '"' ? slashes * 2 + 1 : slashes);
                result.Append(c);
                slashes = 0;
            }
            result.Append('\\', slashes * 2);
            return result.Append('"').ToString();
        }

        public static Dictionary<string, object> Probe(string root)
        {
            root = Path.GetFullPath(root);
            UpdateEngine.NoLinks(Path.Combine(root, "python.exe"));
            string cache = Path.Combine(Path.GetTempPath(), "EmPythonProbe-" + Guid.NewGuid().ToString("N"));
            Directory.CreateDirectory(cache);
            try
            {
                var start = new ProcessStartInfo(Path.Combine(root, "python.exe"), "-I -B -c " + Quote(ProbeCode)) {
                    WorkingDirectory = root, UseShellExecute = false, CreateNoWindow = true, RedirectStandardOutput = true, RedirectStandardError = true
                };
                start.EnvironmentVariables["MPLCONFIGDIR"] = cache;
                using (Process process = Process.Start(start))
                {
                    Task<string> stdout = process.StandardOutput.ReadToEndAsync();
                    Task<string> stderr = process.StandardError.ReadToEndAsync();
                    if (!process.WaitForExit(60000)) { process.Kill(); process.WaitForExit(); throw new IOException("Проверка Python превысила время ожидания."); }
                    UpdateEngine.Require(process.ExitCode == 0, "Проверка Python/NumPy/SciPy/pandas/Matplotlib/Tkinter не пройдена: " + stderr.Result.Substring(0, Math.Min(stderr.Result.Length, 1200)));
                    var result = UpdateEngine.Json.Deserialize<Dictionary<string, object>>(stdout.Result);
                    UpdateEngine.Require(result != null && Convert.ToInt32(result["bits"]) == 64 && Convert.ToBoolean(result["portable"]), "Требуется автономный Python x64, все модули должны находиться внутри Runtime.");
                    return result;
                }
            }
            finally { Directory.Delete(cache, true); }
        }

        internal static void CheckPython(string target)
        {
            string root = Path.Combine(Path.GetFullPath(target), DirectoryName) + Path.DirectorySeparatorChar;
            foreach (string name in new[] { "python", "pythonw", "python3", "python3w" })
                foreach (var process in Process.GetProcessesByName(name))
                    using (process)
                    {
                        try { UpdateEngine.Require(!process.MainModule.FileName.StartsWith(root, StringComparison.OrdinalIgnoreCase), "Закройте процессы локального Python перед обновлением Runtime."); }
                        catch (System.ComponentModel.Win32Exception) { throw new IOException("Не удалось проверить процессы Python."); }
                        catch (InvalidOperationException) { }
                    }
        }

        static void Restore(string backup, RuntimeJournal journal)
        {
            string current = Path.Combine(journal.target, DirectoryName);
            string previous = Path.Combine(backup, DirectoryName);
            UpdateEngine.NoLinks(current);
            UpdateEngine.NoLinks(previous);
            UpdateEngine.NoLinks(journal.incoming);
            if (Directory.Exists(previous))
            {
                if (Directory.Exists(current)) Directory.Move(current, Path.Combine(backup, "failed-new-runtime"));
                Directory.Move(previous, current);
            }
            else if (!journal.had_old && Directory.Exists(current)) Directory.Move(current, Path.Combine(backup, "failed-new-runtime"));
            else UpdateEngine.Require(!journal.had_old || Directory.Exists(current), "Предыдущая среда Python отсутствует; проверьте резервную копию.");
            if (Directory.Exists(journal.incoming)) Directory.Move(journal.incoming, Path.Combine(backup, "incomplete-new-runtime"));
            journal.status = "rolled_back";
            UpdateEngine.WriteJson(Path.Combine(backup, "transaction.json"), journal);
        }

        public static void Recover(string backup)
        {
            UpdateEngine.NoLinks(backup);
            RuntimeJournal journal = UpdateEngine.ReadJson<RuntimeJournal>(Path.Combine(backup, "transaction.json"));
            UpdateEngine.Require(journal != null && journal.component == Component && journal.status == "applying"
                && UpdateEngine.Same(Path.GetDirectoryName(Path.GetDirectoryName(backup)), journal.target)
                && Path.GetFileName(Path.GetDirectoryName(backup)) == ".em-update-backup"
                && UpdateEngine.Same(Path.GetDirectoryName(journal.incoming), journal.target)
                && Path.GetFileName(journal.incoming).StartsWith(".py-new-", StringComparison.Ordinal), "Некорректный журнал восстановления Runtime.");
            using (var mutex = UpdateEngine.Acquire(journal.target))
            {
                UpdateEngine.CheckOtherInstances(journal.target);
                CheckPython(journal.target);
                Restore(backup, journal);
            }
        }

        public static string Apply(InstallJob job)
        {
            UpdateEngine.Require(job != null && job.component == Component && Directory.Exists(job.target) && Directory.Exists(job.stage)
                && !UpdateEngine.Same(job.target, job.stage), "Некорректное задание Runtime.");
            job.target = Path.GetFullPath(job.target);
            UpdateEngine.NoLinks(job.target);
            UpdateEngine.NoLinks(job.stage);
            UpdateEngine.WaitForParent(job);
            using (var mutex = UpdateEngine.Acquire(job.target))
            {
                UpdateEngine.CheckOtherInstances(job.target);
                CheckPython(job.target);
                UpdateEngine.Require(UpdateEngine.PendingBackup(job.target) == null, "Есть незавершенная установка.");
                Manifest m = ReadManifest(Path.Combine(job.stage, "manifest.json"));
                UpdateEngine.Require(m.build > CurrentBuild(job.target), "Установлена более новая или такая же среда.");
                foreach (PayloadFile file in m.files) UpdateEngine.VerifyFile(Path.Combine(job.stage, file.path.Replace('/', Path.DirectorySeparatorChar)), file.size, file.sha256);
                string current = Path.Combine(job.target, DirectoryName);
                UpdateEngine.NoLinks(current);
                UpdateEngine.Require(!File.Exists(current), "Путь python_runtime занят файлом.");
                string backupRoot = Path.Combine(job.target, ".em-update-backup");
                UpdateEngine.NoLinks(backupRoot);
                string backup = Path.Combine(backupRoot, Guid.NewGuid().ToString("N"));
                Directory.CreateDirectory(backup);
                string incoming = Path.Combine(job.target, ".py-new-" + Guid.NewGuid().ToString("N").Substring(0, 8));
                var journal = new RuntimeJournal { target = job.target, incoming = incoming, had_old = Directory.Exists(current), status = "applying" };
                UpdateEngine.WriteJson(Path.Combine(backup, "transaction.json"), journal);
                try
                {
                    Directory.CreateDirectory(incoming);
                    foreach (PayloadFile file in m.files)
                    {
                        string destination = Path.Combine(incoming, file.path.Replace('/', Path.DirectorySeparatorChar));
                        Directory.CreateDirectory(Path.GetDirectoryName(destination));
                        File.Copy(Path.Combine(job.stage, file.path.Replace('/', Path.DirectorySeparatorChar)), destination, false);
                        UpdateEngine.VerifyFile(destination, file.size, file.sha256);
                    }
                    var probe = Probe(incoming);
                    UpdateEngine.Require(Convert.ToString(probe["python"]) == m.version && Packages.All(p => Convert.ToString(probe[p]) == m.packages[p]), "Версии Python и библиотек не совпадают с манифестом.");
                    UpdateEngine.WriteJson(Path.Combine(incoming, State), m);
                    if (journal.had_old) Directory.Move(current, Path.Combine(backup, DirectoryName));
                    Directory.Move(incoming, current);
                    probe = Probe(current);
                    UpdateEngine.Require(Convert.ToString(probe["python"]) == m.version && Packages.All(p => Convert.ToString(probe[p]) == m.packages[p]), "Среда Python не прошла проверку после переноса.");
                    journal.status = "installed";
                    UpdateEngine.WriteJson(Path.Combine(backup, "transaction.json"), journal);
                    if (job.restart) Process.Start(new ProcessStartInfo(Path.Combine(job.target, UpdateEngine.App)) { UseShellExecute = false, WorkingDirectory = job.target });
                    return backup;
                }
                catch (Exception error)
                {
                    try { Restore(backup, journal); }
                    catch (Exception rollback) { throw new IOException("Ошибка Runtime и восстановления; резервная копия: " + backup + ". " + rollback.Message, error); }
                    throw new IOException("Python Runtime не установлен; предыдущая среда сохранена/восстановлена. " + error.Message, error);
                }
            }
        }
    }
}
