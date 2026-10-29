using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Drawing;
using System.IO;
using System.IO.Compression;
using System.Linq;
using System.Net;
using System.Reflection;
using System.Security.Cryptography;
using System.Text;
using System.Threading.Tasks;
using System.Web.Script.Serialization;
using System.Windows.Forms;

[assembly: AssemblyTitle("Electromagnetic Tools Publish Agent")]
[assembly: AssemblyCompany("НПФ «АМК Горизонт»")]
[assembly: AssemblyCopyright("Организация: НПФ «АМК Горизонт», разработал зам начальника ОП Смирнов С. Г.")]
[assembly: AssemblyVersion("1.0.0.0")]

namespace EmTools.Publish
{
    public sealed class UpdateSettings
    {
        public bool automatic;
        public string base_uri = "ftp://89.189.156.184/VORONEJ_FTP/UPDATE/ELECTROMAGNETIC_TOOLS_3.0/";
        public string username = "";
        public string protected_password = "";

        public string Password()
        {
            return String.IsNullOrEmpty(protected_password) ? "" : Encoding.UTF8.GetString(ProtectedData.Unprotect(Convert.FromBase64String(protected_password), null, DataProtectionScope.CurrentUser));
        }

        public void SetPassword(string value)
        {
            protected_password = String.IsNullOrEmpty(value) ? "" : Convert.ToBase64String(ProtectedData.Protect(Encoding.UTF8.GetBytes(value), null, DataProtectionScope.CurrentUser));
        }
    }

    public sealed class PayloadFile
    {
        public string path;
        public long size;
        public string sha256;
    }

    public sealed class Manifest
    {
        public int schema;
        public string product;
        public string platform;
        public string authentication;
        public string component = "application";
        public Dictionary<string, string> packages;
        public long build;
        public string version;
        public string released_utc;
        public string notes;
        public string package;
        public long size;
        public string sha256;
        public PayloadFile[] files;
        public string organization;
        public string author;
    }

    internal static class PublishEngine
    {
        internal const string Product = "ELECTROMAGNETIC_TOOLS_3.0";
        internal const string App = Product + ".exe";
        internal const string Agent = "EmUpdate.exe";
        internal const string Organization = "НПФ «АМК Горизонт»";
        internal const string Author = "Разработал зам начальника ОП Смирнов С. Г.";
        internal const long MaxPackage = 256L * 1024 * 1024;
        internal const long MaxPayload = 536870912L;
        internal const int MaxRuntimeFiles = 20000;
        internal const long MaxRuntimeExpanded = 1073741824L;
        internal const int MaxAppManifest = 1048576;
        internal const int MaxRuntimeManifest = 16777216;
        internal static readonly string[] AllowedFiles = { App, Agent, App + ".config", Agent + ".config" };
        internal static readonly string[] RuntimePackages = { "numpy", "scipy", "pandas", "matplotlib", "tcl" };
        internal static JavaScriptSerializer Json { get { return new JavaScriptSerializer { MaxJsonLength = 32 * 1024 * 1024 }; } }

        internal static void Require(bool condition, string message)
        {
            if (!condition) throw new InvalidDataException(message);
        }

        internal static string Hash(string path)
        {
            using (var stream = File.OpenRead(path))
            using (var sha = SHA256.Create())
                return BitConverter.ToString(sha.ComputeHash(stream)).Replace("-", "").ToLowerInvariant();
        }

        internal static string Hash(byte[] data)
        {
            using (var sha = SHA256.Create())
                return BitConverter.ToString(sha.ComputeHash(data)).Replace("-", "").ToLowerInvariant();
        }

        internal static string Timestamp() { return DateTime.UtcNow.ToString("o"); }

        static void NoLinks(string path)
        {
            Require((File.GetAttributes(path) & FileAttributes.ReparsePoint) == 0, "Ссылки/reparse points в путях публикации запрещены: " + path);
        }

        internal static string SettingsPath(string target)
        {
            string id;
            using (var sha = SHA256.Create()) id = BitConverter.ToString(sha.ComputeHash(Encoding.UTF8.GetBytes(target.ToUpperInvariant()))).Replace("-", "").Substring(0, 16);
            return Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData), "AMK_Gorizont", "ElectromagneticTools3", id, "updates.json");
        }

        internal static UpdateSettings LoadSettings(string target)
        {
            string path = SettingsPath(target);
            return File.Exists(path) ? Json.Deserialize<UpdateSettings>(File.ReadAllText(path, Encoding.UTF8)) : new UpdateSettings();
        }

        internal static void SaveSettings(string target, UpdateSettings settings)
        {
            string path = SettingsPath(target);
            Directory.CreateDirectory(Path.GetDirectoryName(path));
            File.WriteAllText(path, Json.Serialize(settings), new UTF8Encoding(false));
        }

        internal static FtpWebRequest Ftp(Uri uri, UpdateSettings settings, string method)
        {
            var request = (FtpWebRequest)WebRequest.Create(uri);
            request.Method = method;
            request.Proxy = null;
            request.Credentials = new NetworkCredential(settings.username, settings.Password());
            request.UseBinary = true;
            request.UsePassive = true;
            request.KeepAlive = false;
            request.Timeout = 30000;
            request.ReadWriteTimeout = 30000;
            return request;
        }

        internal static byte[] Download(Uri uri, UpdateSettings settings, int limit, out bool missing)
        {
            missing = false;
            try
            {
                using (var response = (FtpWebResponse)Ftp(uri, settings, WebRequestMethods.Ftp.DownloadFile).GetResponse())
                using (var input = response.GetResponseStream())
                using (var output = new MemoryStream())
                {
                    byte[] buffer = new byte[65536];
                    int count;
                    while ((count = input.Read(buffer, 0, buffer.Length)) > 0)
                    {
                        output.Write(buffer, 0, count);
                        Require(output.Length <= limit, "Удаленный манифест слишком большой.");
                    }
                    return output.ToArray();
                }
            }
            catch (WebException error)
            {
                var response = error.Response as FtpWebResponse;
                if (response != null && response.StatusCode == FtpStatusCode.ActionNotTakenFileUnavailable) { missing = true; return null; }
                throw;
            }
        }

        internal static void Upload(Uri directory, UpdateSettings settings, string local, string remote)
        {
            string temporary = ".upload-" + Guid.NewGuid().ToString("N") + Path.GetExtension(remote);
            var upload = Ftp(new Uri(directory, temporary), settings, WebRequestMethods.Ftp.UploadFile);
            using (var source = File.OpenRead(local))
            using (var stream = upload.GetRequestStream())
                source.CopyTo(stream, 262144);
            using (upload.GetResponse()) { }
            var rename = Ftp(new Uri(directory, temporary), settings, WebRequestMethods.Ftp.Rename);
            rename.RenameTo = remote;
            using (rename.GetResponse()) { }
        }

        internal static void VerifyRemoteManifest(Uri directory, UpdateSettings settings, string manifestName, Manifest manifest, int limit)
        {
            bool missing;
            byte[] current = Download(new Uri(directory, manifestName), settings, limit, out missing);
            if (missing || current == null || current.Length == 0) return;
            var previous = Json.Deserialize<Manifest>(Encoding.UTF8.GetString(current).TrimStart('﻿'));
            Require(previous != null && previous.product == manifest.product, "Удаленный манифест принадлежит другому продукту.");
            string component = previous.component ?? "application";
            Require(component == manifest.component, "Удаленный манифест принадлежит другому каналу: " + component);
            Require(previous.build < manifest.build, "Номер сборки должен быть больше опубликованного (" + previous.build + ").");
        }

        static string RunAgent(string agent, string arguments)
        {
            var info = new ProcessStartInfo(agent, arguments) { UseShellExecute = false, RedirectStandardOutput = true, CreateNoWindow = true, StandardOutputEncoding = Encoding.UTF8 };
            using (var process = Process.Start(info))
            {
                string output = process.StandardOutput.ReadToEnd();
                if (!process.WaitForExit(240000)) { try { process.Kill(); } catch { } throw new IOException("Проверка EmUpdate.exe завершилась по таймауту."); }
                Require(process.ExitCode == 0, "EmUpdate.exe вернул ошибку: " + output.Trim());
                return output;
            }
        }

        internal static Manifest PackageApplication(string source, string output, long build, string notes, Action<string> log)
        {
            string agent = Path.Combine(source, Agent);
            string app = Path.Combine(source, App);
            Require(build > 0, "Номер сборки должен быть положительным.");
            Require(notes != null && notes.Length <= 16000, "Описание слишком длинное.");
            Require(File.Exists(app) && File.Exists(agent), "В x64\\Release нет главного EXE или EmUpdate.exe.");
            Require(Directory.Exists(Path.GetDirectoryName(output)), "Родительская папка вывода не существует.");
            Directory.CreateDirectory(output);
            string version = RunAgent(agent, "--info \"" + app + "\"").Trim();
            log("Версия сборки: " + version);
            string package = Path.Combine(output, "release-" + build + "-" + Guid.NewGuid().ToString("N").Substring(0, 12) + ".zip");
            var files = new List<PayloadFile>();
            using (var zip = new ZipArchive(File.Create(package), ZipArchiveMode.Create))
            {
                foreach (string name in AllowedFiles)
                {
                    string path = Path.Combine(source, name);
                    if (!File.Exists(path)) continue;
                    NoLinks(path);
                    byte[] data = File.ReadAllBytes(path);
                    Require(data.LongLength <= MaxPackage, "Файл слишком большой: " + name);
                    var entry = zip.CreateEntry(name);
                    using (var stream = entry.Open()) stream.Write(data, 0, data.Length);
                    files.Add(new PayloadFile { path = name, size = data.LongLength, sha256 = Hash(data) });
                    log("  + " + name + " (" + data.LongLength + " байт)");
                }
                string scriptsDir = Path.Combine(source, "scripts");
                if (Directory.Exists(scriptsDir))
                {
                    foreach (string path in Directory.GetFiles(scriptsDir, "*", SearchOption.AllDirectories).OrderBy(f => f, StringComparer.Ordinal))
                    {
                        string relative = path.Substring(scriptsDir.Length + 1).Replace('\\', '/');
                        if (relative.IndexOf("__pycache__", StringComparison.Ordinal) >= 0) continue;
                        string name = "scripts/" + relative;
                        NoLinks(path);
                        Require(path.EndsWith(".py", StringComparison.Ordinal), "В scripts допускаются только файлы *.py: " + name);
                        byte[] data = File.ReadAllBytes(path);
                        Require(data.LongLength <= MaxPackage, "Файл слишком большой: " + name);
                        var entry = zip.CreateEntry(name);
                        using (var stream = entry.Open()) stream.Write(data, 0, data.Length);
                        files.Add(new PayloadFile { path = name, size = data.LongLength, sha256 = Hash(data) });
                        log("  + " + name + " (" + data.LongLength + " байт)");
                    }
                }
            }
            long size = new FileInfo(package).Length;
            Require(size <= MaxPackage && files.Sum(f => f.size) <= MaxPayload, "Пакет превышает лимиты updater.");
            var manifest = new Manifest
            {
                schema = 1, product = Product, platform = "x64", authentication = "none", component = "application",
                build = build, version = version, released_utc = Timestamp(), notes = notes,
                package = Path.GetFileName(package), size = size, sha256 = Hash(package), files = files.ToArray(),
                organization = Organization, author = Author
            };
            File.WriteAllText(Path.Combine(output, "manifest.json"), Json.Serialize(manifest), new UTF8Encoding(false));
            return manifest;
        }

        static IEnumerable<string> RuntimeFiles(string runtime)
        {
            var queue = new Stack<string>();
            queue.Push(runtime);
            while (queue.Count > 0)
            {
                string current = queue.Pop();
                var directories = Directory.GetDirectories(current)
                    .Where(d => { string n = Path.GetFileName(d); return n != "__pycache__" && n != "tests" && n != "test" && n != ".git"; })
                    .OrderBy(d => d, StringComparer.Ordinal).ToArray();
                foreach (string directory in directories) { NoLinks(directory); queue.Push(directory); }
                foreach (string file in Directory.GetFiles(current).OrderBy(f => f, StringComparer.Ordinal))
                {
                    NoLinks(file);
                    string name = Path.GetFileName(file);
                    if (name.EndsWith(".pyc", StringComparison.OrdinalIgnoreCase) || name.EndsWith(".pyo", StringComparison.OrdinalIgnoreCase)
                        || name.EndsWith(".chm", StringComparison.OrdinalIgnoreCase) || name == "runtime-release.json") continue;
                    yield return file;
                }
            }
        }

        internal static Manifest PackageRuntime(string source, string output, long build, string notes, Action<string> log)
        {
            string agent = Path.Combine(source, Agent);
            string runtime = Path.Combine(source, "python_runtime");
            Require(build > 0, "Номер сборки должен быть положительным.");
            Require(notes != null && notes.Length <= 16000, "Описание слишком длинное.");
            Require(File.Exists(agent), "В x64\\Release нет EmUpdate.exe.");
            Require(Directory.Exists(runtime), "В x64\\Release нет папки python_runtime.");
            Require(!output.StartsWith(runtime + "\\", StringComparison.OrdinalIgnoreCase), "Вывод должен быть вне среды Runtime.");
            Require(Directory.Exists(Path.GetDirectoryName(output)), "Родительская папка вывода не существует.");
            log("Проверка среды через EmUpdate --python-probe...");
            var probe = Json.Deserialize<Dictionary<string, object>>(RunAgent(agent, "--python-probe \"" + runtime + "\"").Trim());
            Require(probe.ContainsKey("bits") && Convert.ToInt32(probe["bits"]) == 64 && probe.ContainsKey("portable") && (bool)probe["portable"], "Среда не portable x64.");
            foreach (string name in RuntimePackages) Require(probe.ContainsKey(name), "В среде нет пакета " + name + ".");
            Directory.CreateDirectory(output);
            string package = Path.Combine(output, "python-runtime-" + build + "-" + Guid.NewGuid().ToString("N").Substring(0, 12) + ".zip");
            var files = new List<PayloadFile>();
            long total = 0;
            using (var zip = new ZipArchive(File.Create(package), ZipArchiveMode.Create))
            {
                foreach (string path in RuntimeFiles(runtime))
                {
                    string relative = path.Substring(runtime.Length + 1).Replace('\\', '/');
                    long length = new FileInfo(path).Length;
                    Require(length <= MaxPackage, "Файл Runtime слишком большой: " + relative);
                    var entry = zip.CreateEntry(relative);
                    var sha = SHA256.Create();
                    using (var input = File.OpenRead(path))
                    using (var stream = new CryptoStream(entry.Open(), sha, CryptoStreamMode.Write))
                        input.CopyTo(stream, 262144);
                    files.Add(new PayloadFile { path = relative, size = length, sha256 = BitConverter.ToString(sha.Hash).Replace("-", "").ToLowerInvariant() });
                    sha.Dispose();
                    total += length;
                    Require(files.Count <= MaxRuntimeFiles && total <= MaxRuntimeExpanded, "Runtime превышает лимиты файлов/размера.");
                }
            }
            long size = new FileInfo(package).Length;
            Require(size <= MaxPackage, "Runtime ZIP превышает 256 MiB.");
            log("Файлов в пакете: " + files.Count + ", архив: " + size + " байт");
            var manifest = new Manifest
            {
                schema = 1, product = Product, platform = "x64", authentication = "none", component = "python-runtime",
                build = build, version = Convert.ToString(probe["python"]), released_utc = Timestamp(), notes = notes,
                package = Path.GetFileName(package), size = size, sha256 = Hash(package), files = files.ToArray(),
                packages = RuntimePackages.ToDictionary(n => n, n => Convert.ToString(probe[n])),
                organization = Organization, author = Author
            };
            string manifestPath = Path.Combine(output, "python-runtime.json");
            File.WriteAllText(manifestPath, Json.Serialize(manifest), new UTF8Encoding(false));
            log("Проверка пакета через EmUpdate --runtime-prepare...");
            string target = Path.Combine(Path.GetTempPath(), "EmPublishCheck-" + Guid.NewGuid().ToString("N"));
            Directory.CreateDirectory(target);
            try { RunAgent(agent, "--runtime-prepare \"" + manifestPath + "\" \"" + package + "\" \"" + target + "\""); }
            finally { try { Directory.Delete(target, true); } catch { } }
            return manifest;
        }

        internal static void Publish(Uri directory, UpdateSettings settings, string manifestName, Manifest manifest, string output, Action<string> log)
        {
            Require(manifestName == "manifest.json" || manifestName == "python-runtime.json", "Неподдерживаемый канал публикации.");
            log("Проверка удаленного манифеста...");
            VerifyRemoteManifest(directory, settings, manifestName, manifest, manifestName == "python-runtime.json" ? MaxRuntimeManifest : MaxAppManifest);
            log("Загрузка архива " + manifest.package + "...");
            Upload(directory, settings, Path.Combine(output, manifest.package), manifest.package);
            log("Загрузка " + manifestName + " (последним)...");
            Upload(directory, settings, Path.Combine(output, manifestName), manifestName);
        }
    }

    public sealed class PublishForm : Form
    {
        readonly ComboBox channel;
        readonly TextBox build;
        readonly TextBox notes;
        readonly TextBox uriBox, userBox, passBox;
        readonly TextBox log;
        readonly Button pack, publish;
        readonly string source;
        UpdateSettings settings;
        Manifest manifest;
        string output;

        public PublishForm()
        {
            source = Application.StartupPath;
            Text = "Electromagnetic Tools — публикация релиза (НПФ «АМК Горизонт»)";
            Width = 720; Height = 640; StartPosition = FormStartPosition.CenterScreen; MinimizeBox = false;

            var top = new Panel { Dock = DockStyle.Top, Height = 210 };
            top.Controls.Add(new Label { Left = 12, Top = 15, Width = 100, Text = "Канал:" });
            channel = new ComboBox { Left = 115, Top = 12, Width = 300, DropDownStyle = ComboBoxStyle.DropDownList };
            channel.Items.AddRange(new object[] { "Приложение (manifest.json)", "Python Runtime (python-runtime.json)" });
            channel.SelectedIndex = 0;
            top.Controls.Add(channel);
            top.Controls.Add(new Label { Left = 425, Top = 15, Width = 60, Text = "Сборка:" });
            build = new TextBox { Left = 490, Top = 12, Width = 80 };
            top.Controls.Add(build);
            top.Controls.Add(new Label { Left = 12, Top = 45, Width = 100, Text = "Источник:" });
            var src = new Label { Left = 115, Top = 45, Width = 580, Text = source + "  (фиксировано)" };
            top.Controls.Add(src);
            top.Controls.Add(new Label { Left = 12, Top = 75, Width = 100, Text = "Описание:" });
            notes = new TextBox { Left = 115, Top = 72, Width = 580, Height = 55, Multiline = true, ScrollBars = ScrollBars.Vertical };
            top.Controls.Add(notes);
            top.Controls.Add(new Label { Left = 12, Top = 140, Width = 100, Text = "FTP URI:" });
            uriBox = new TextBox { Left = 115, Top = 137, Width = 460 };
            top.Controls.Add(uriBox);
            top.Controls.Add(new Label { Left = 12, Top = 170, Width = 100, Text = "Логин/пароль:" });
            userBox = new TextBox { Left = 115, Top = 167, Width = 220 };
            passBox = new TextBox { Left = 345, Top = 167, Width = 230, UseSystemPasswordChar = true };
            top.Controls.Add(userBox); top.Controls.Add(passBox);
            var saveCred = new Button { Left = 585, Top = 165, Width = 110, Text = "Сохранить доступ" };
            saveCred.Click += (s, e) =>
            {
                try
                {
                    var next = new UpdateSettings { base_uri = uriBox.Text.Trim().TrimEnd('/') + "/", username = userBox.Text.Trim(), automatic = settings.automatic };
                    next.SetPassword(passBox.Text);
                    PublishEngine.SaveSettings(source, next);
                    settings = next;
                    Log("Доступ сохранен локально (DPAPI).");
                }
                catch (Exception ex) { MessageBox.Show(this, ex.Message, "Доступ FTP", MessageBoxButtons.OK, MessageBoxIcon.Warning); }
            };
            top.Controls.Add(saveCred);

            log = new TextBox { Dock = DockStyle.Fill, Multiline = true, ReadOnly = true, ScrollBars = ScrollBars.Vertical, Font = new Font("Consolas", 9f), BackColor = Color.White };

            var bottom = new Panel { Dock = DockStyle.Bottom, Height = 48 };
            pack = new Button { Left = 12, Top = 10, Width = 220, Text = "1. Собрать пакет (локально)" };
            publish = new Button { Left = 245, Top = 10, Width = 220, Text = "2. Опубликовать на FTP", Enabled = false };
            bottom.Controls.Add(pack); bottom.Controls.Add(publish);

            Controls.Add(log); Controls.Add(bottom); Controls.Add(top);

            settings = PublishEngine.LoadSettings(source);
            uriBox.Text = settings.base_uri;
            userBox.Text = settings.username;
            Log("Источник публикации: " + source);
            if (!File.Exists(Path.Combine(source, PublishEngine.App)) || !File.Exists(Path.Combine(source, PublishEngine.Agent)))
                Log("ВНИМАНИЕ: EmPublish.exe должен лежать в x64\\Release рядом с программой и EmUpdate.exe.");

            pack.Click += async (s, e) => await Pack();
            publish.Click += async (s, e) => await Publish();
        }

        void Log(string text) { log.AppendText("[" + DateTime.Now.ToString("HH:mm:ss") + "] " + text + Environment.NewLine); }

        async Task Pack()
        {
            long number;
            if (!Int64.TryParse(build.Text.Trim(), out number) || number <= 0) { MessageBox.Show(this, "Укажите положительный номер сборки.", "Публикация"); return; }
            pack.Enabled = false; publish.Enabled = false;
            bool runtime = channel.SelectedIndex == 1;
            output = Path.Combine(Path.GetDirectoryName(source.TrimEnd('\\')), runtime ? "PythonRuntimePackage-" + number : "ReleasePackage-" + number);
            try
            {
                manifest = await Task.Run(() => runtime
                    ? PublishEngine.PackageRuntime(source, output, number, notes.Text.Trim(), t => BeginInvoke((Action)(() => Log(t))))
                    : PublishEngine.PackageApplication(source, output, number, notes.Text.Trim(), t => BeginInvoke((Action)(() => Log(t)))));
                Log("Пакет собран: " + output);
                Log("  " + manifest.package + " — " + manifest.size + " байт, SHA-256 " + manifest.sha256);
                publish.Enabled = true;
            }
            catch (Exception ex) { Log("ОШИБКА: " + ex.Message); manifest = null; }
            finally { pack.Enabled = true; }
        }

        async Task Publish()
        {
            if (manifest == null) return;
            if (MessageBox.Show(this, "Опубликовать " + manifest.package + " (build " + manifest.build + ") на FTP?\nАрхив будет загружен под временным именем, затем переименован; манифест — последним.",
                "Публикация", MessageBoxButtons.YesNo, MessageBoxIcon.Question) != DialogResult.Yes) return;
            publish.Enabled = false; pack.Enabled = false;
            var channelSettings = new UpdateSettings { base_uri = uriBox.Text.Trim().TrimEnd('/') + "/", username = userBox.Text.Trim() };
            channelSettings.SetPassword(passBox.Text);
            string manifestName = manifest.component == "python-runtime" ? "python-runtime.json" : "manifest.json";
            try
            {
                await Task.Run(() => PublishEngine.Publish(new Uri(channelSettings.base_uri), channelSettings, manifestName, manifest, output, t => BeginInvoke((Action)(() => Log(t)))));
                Log("ПУБЛИКАЦИЯ ЗАВЕРШЕНА: " + manifestName + " обновлен последним.");
            }
            catch (Exception ex) { Log("ОШИБКА ПУБЛИКАЦИИ: " + ex.Message + " — локальный пакет сохранен в " + output); }
            finally { pack.Enabled = true; }
        }

        [STAThread]
        public static int Main(string[] args)
        {
            if (args.Length == 0)
            {
                Application.EnableVisualStyles();
                Application.Run(new PublishForm());
                return 0;
            }
            try { return Cli.Run(args); }
            catch (Exception ex) { Console.Error.WriteLine("[PUBLISH FAILED] " + ex.Message); return 1; }
        }
    }

    internal static class Cli
    {
        internal static int Run(string[] args)
        {
            string source = Path.GetDirectoryName(Assembly.GetExecutingAssembly().Location);
            bool runtime = args.Contains("--runtime");
            long build = Value<long>(args, "--build");
            string output = Value<string>(args, "--output");
            string notes = Value<string>(args, "--desc") ?? "";
            bool publish = args.Contains("--publish");
            Action<string> log = t => Console.WriteLine(t);
            Manifest manifest = runtime
                ? PublishEngine.PackageRuntime(source, output, build, notes, log)
                : PublishEngine.PackageApplication(source, output, build, notes, log);
            Console.WriteLine("[PACKAGE SUCCESS] " + manifest.package + " build=" + manifest.build + " bytes=" + manifest.size);
            if (!publish) { Console.WriteLine("[LOCAL ONLY] FTP не использовался"); return 0; }
            var settings = PublishEngine.LoadSettings(source);
            string manifestName = runtime ? "python-runtime.json" : "manifest.json";
            PublishEngine.Publish(new Uri(settings.base_uri), settings, manifestName, manifest, output, log);
            Console.WriteLine("[PUBLISH SUCCESS] " + manifestName + " обновлен последним");
            return 0;
        }

        static T Value<T>(string[] args, string name)
        {
            int index = Array.IndexOf(args, name);
            if (index < 0 || index + 1 >= args.Length)
                throw new InvalidDataException("Требуется аргумент " + name + ". Использование: EmPublish.exe [--runtime] --build N --output DIR --desc TXT [--publish]");
            return (T)Convert.ChangeType(args[index + 1], typeof(T));
        }
    }
}
