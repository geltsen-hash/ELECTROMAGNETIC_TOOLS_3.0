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
using System.Threading;
using System.Threading.Tasks;
using System.Web.Script.Serialization;
using System.Windows.Forms;

[assembly: AssemblyTitle("Electromagnetic Tools Update Agent")]
[assembly: AssemblyCompany("НПФ «АМК Горизонт»")]
[assembly: AssemblyCopyright("Организация: НПФ «АМК Горизонт», разработал зам начальника ОП Смирнов С. Г.")]
[assembly: AssemblyVersion("1.0.0.0")]

namespace EmTools.Updates
{
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
    }

    public sealed class InstallJob
    {
        public string component = "application";
        public string stage;
        public string target;
        public int parent_pid;
        public long parent_start_ticks;
        public bool restart;
    }

    public sealed class Snapshot
    {
        public string path;
        public bool existed;
        public string sha256;
    }

    public sealed class Journal
    {
        public string component = "application";
        public string target;
        public string status;
        public Snapshot[] files;
        public string[] applied = new string[0];
        public string pending;
    }

    internal sealed class UpdateLock : IDisposable
    {
        readonly Mutex mutex;
        internal UpdateLock(Mutex mutex) { this.mutex = mutex; }
        public void Dispose() { mutex.ReleaseMutex(); mutex.Dispose(); }
    }

    public static class UpdateEngine
    {
        public const string Product = "ELECTROMAGNETIC_TOOLS_3.0";
        public const string App = "ELECTROMAGNETIC_TOOLS_3.0.exe";
        public const string Agent = "EmUpdate.exe";
        public const string State = "release.json";
        public const long MaxPackage = 256L * 1024 * 1024;
        public static readonly string[] AllowedFiles = { App, Agent, App + ".config", Agent + ".config" };

        internal static bool IsAllowedFile(string path)
        {
            if (AllowedFiles.Contains(path, StringComparer.Ordinal)) return true;
            return path.StartsWith("scripts/", StringComparison.Ordinal) && path.IndexOf('\\') < 0
                && path.EndsWith(".py", StringComparison.Ordinal) && path.IndexOf("..") < 0
                && path.IndexOf("__pycache__", StringComparison.Ordinal) < 0;
        }
        internal static JavaScriptSerializer Json { get { return new JavaScriptSerializer { MaxJsonLength = 16 * 1024 * 1024 }; } }

        internal static void Require(bool condition, string message)
        {
            if (!condition) throw new InvalidDataException(message);
        }

        internal static bool Same(string left, string right)
        {
            return String.Equals(Path.GetFullPath(left).TrimEnd('\\'), Path.GetFullPath(right).TrimEnd('\\'), StringComparison.OrdinalIgnoreCase);
        }

        internal static void NoLinks(string path)
        {
            string current = Path.GetFullPath(path);
            while (!String.IsNullOrEmpty(current))
            {
                if (File.Exists(current) || Directory.Exists(current))
                    Require((File.GetAttributes(current) & FileAttributes.ReparsePoint) == 0, "Ссылки/reparse points в путях обновления запрещены.");
                current = Path.GetDirectoryName(current);
            }
        }

        internal static string Hash(string path)
        {
            using (var stream = File.OpenRead(path))
            using (var sha = SHA256.Create())
                return BitConverter.ToString(sha.ComputeHash(stream)).Replace("-", "").ToLowerInvariant();
        }

        internal static bool ValidHash(string value)
        {
            return value != null && value.Length == 64 && value.All(c => (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'));
        }

        internal static T ReadJson<T>(string path)
        {
            Require(new FileInfo(path).Length <= 1024 * 1024, "JSON слишком велик.");
            return Json.Deserialize<T>(File.ReadAllText(path, Encoding.UTF8));
        }

        internal static void WriteJson(string path, object data)
        {
            string temporary = path + ".em-json-" + Guid.NewGuid().ToString("N");
            try
            {
                byte[] bytes = new UTF8Encoding(false).GetBytes(Json.Serialize(data));
                using (var file = new FileStream(temporary, FileMode.CreateNew, FileAccess.Write, FileShare.None))
                {
                    file.Write(bytes, 0, bytes.Length);
                    file.Flush(true);
                }
                if (File.Exists(path)) File.Replace(temporary, path, null);
                else File.Move(temporary, path);
            }
            finally { if (File.Exists(temporary)) File.Delete(temporary); }
        }

        public static Manifest ReadManifest(string path)
        {
            Manifest m = ReadJson<Manifest>(path);
            Require(m != null && m.schema == 1 && m.product == Product && m.platform == "x64" && m.component == "application", "Пакет другого продукта, платформы или формата.");
            Require(m.authentication == "none", "Неподдерживаемый режим аутентификации пакета.");
            Version version;
            DateTime released;
            Require(m.build > 0 && Version.TryParse(m.version, out version) && DateTime.TryParse(m.released_utc, System.Globalization.CultureInfo.InvariantCulture,
                System.Globalization.DateTimeStyles.RoundtripKind, out released), "Некорректная версия/дата выпуска.");
            Require(m.notes != null && m.notes.Length <= 16000, "Некорректное описание релиза.");
            Require(!String.IsNullOrEmpty(m.package) && m.package == Path.GetFileName(m.package) && m.package.StartsWith("release-", StringComparison.Ordinal)
                && m.package.EndsWith(".zip", StringComparison.Ordinal) && m.package.All(c => Char.IsLetterOrDigit(c) && c < 128 || c == '-' || c == '.'), "Некорректное имя ZIP.");
            Require(m.size > 0 && m.size <= MaxPackage && ValidHash(m.sha256), "Некорректный размер или SHA-256 ZIP.");
            Require(m.files != null && m.files.Length >= 2 && m.files.Length <= 64, "Некорректный состав пакета.");
            var names = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
            long total = 0;
            foreach (PayloadFile file in m.files)
            {
                Require(file != null && IsAllowedFile(file.path) && names.Add(file.path), "Недопустимый или повторяющийся путь в пакете.");
                Require(file.size > 0 && file.size <= MaxPackage && ValidHash(file.sha256), "Некорректное описание файла пакета.");
                total += file.size;
            }
            Require(total <= MaxPackage * 2 && names.Contains(App) && names.Contains(Agent), "В пакете должны быть EXE приложения и updater.");
            return m;
        }

        public static long CurrentBuild(string target)
        {
            string path = Path.Combine(target, State);
            if (!File.Exists(path)) return 0;
            Manifest m = ReadManifest(path);
            return m.build;
        }

        public static string PendingBackup(string target)
        {
            string root = Path.Combine(target, ".em-update-backup");
            NoLinks(root);
            if (!Directory.Exists(root)) return null;
            foreach (string directory in Directory.GetDirectories(root))
            {
                NoLinks(directory);
                string transaction = Path.Combine(directory, "transaction.json");
                NoLinks(transaction);
                if (File.Exists(transaction) && ReadJson<Journal>(transaction).status == "applying") return directory;
            }
            return null;
        }

        internal static void VerifyFile(string path, long size, string hash)
        {
            NoLinks(path);
            Require(File.Exists(path) && new FileInfo(path).Length == size && Hash(path) == hash, "Размер или SHA-256 не совпадает: " + Path.GetFileName(path));
        }

        public static string Prepare(string manifestPath, string zipPath, string target)
        {
            NoLinks(target);
            Require(Directory.Exists(target), "Папка приложения не найдена.");
            Require(PendingBackup(target) == null, "Найдена незавершенная установка. Сначала выполните восстановление из резервной копии.");
            Manifest m = ReadManifest(manifestPath);
            Require(m.build > CurrentBuild(target), "Установлена такая же или более новая сборка.");
            VerifyFile(zipPath, m.size, m.sha256);
            string stage = Path.Combine(Path.GetTempPath(), "EmToolsUpdate-" + Guid.NewGuid().ToString("N"));
            Directory.CreateDirectory(stage);
            try
            {
                using (var zip = ZipFile.OpenRead(zipPath))
                {
                    Require(zip.Entries.Count == m.files.Length, "Состав ZIP не совпадает с манифестом.");
                    var seen = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
                    foreach (var entry in zip.Entries)
                    {
                        PayloadFile file = m.files.SingleOrDefault(f => f.path == entry.FullName);
                        Require(file != null && seen.Add(entry.FullName) && entry.Length == file.size, "Лишний, повторный или поврежденный файл ZIP.");
                        Require((entry.ExternalAttributes & 0x400) == 0 && ((entry.ExternalAttributes >> 16) & 0xF000) != 0xA000, "Ссылки в ZIP запрещены.");
                        string output = Path.Combine(stage, file.path);
                        Directory.CreateDirectory(Path.GetDirectoryName(output));
                        using (Stream input = entry.Open())
                        using (Stream destination = new FileStream(output, FileMode.CreateNew, FileAccess.Write, FileShare.None))
                        {
                            byte[] buffer = new byte[65536];
                            long length = 0;
                            int read;
                            while ((read = input.Read(buffer, 0, buffer.Length)) != 0)
                            {
                                length += read;
                                Require(length <= file.size, "Превышен размер распакованного файла.");
                                destination.Write(buffer, 0, read);
                            }
                            Require(length == file.size, "Неполный файл ZIP.");
                        }
                        VerifyFile(output, file.size, file.sha256);
                    }
                }
                WriteJson(Path.Combine(stage, "manifest.json"), m);
                return stage;
            }
            catch
            {
                Directory.Delete(stage, true);
                throw;
            }
        }

        internal static void AtomicCopy(string source, string destination)
        {
            string temp = destination + ".em-new-" + Guid.NewGuid().ToString("N");
            try
            {
                File.Copy(source, temp, false);
                if (File.Exists(destination)) File.Replace(temp, destination, null);
                else File.Move(temp, destination);
            }
            finally { if (File.Exists(temp)) File.Delete(temp); }
        }

        internal static void Rollback(string backup, Journal journal)
        {
            foreach (Snapshot file in journal.files.Reverse().Where(f => journal.applied.Contains(f.path, StringComparer.Ordinal)))
            {
                string destination = Path.Combine(journal.target, file.path);
                NoLinks(destination);
                if (file.existed)
                {
                    string source = Path.Combine(backup, file.path);
                    Require(Hash(source) == file.sha256, "Повреждена резервная копия: " + file.path);
                    AtomicCopy(source, destination);
                }
                else if (File.Exists(destination)) File.Delete(destination);
            }
            journal.status = "rolled_back";
            WriteJson(Path.Combine(backup, "transaction.json"), journal);
        }

        public static void Recover(string backup)
        {
            NoLinks(backup);
            Journal journal = ReadJson<Journal>(Path.Combine(backup, "transaction.json"));
            if (journal != null && journal.component == PythonRuntime.Component) { PythonRuntime.Recover(backup); return; }
            Require(journal != null && journal.status == "applying" && journal.files != null, "Нет незавершенной установки.");
            Require(Same(Path.GetDirectoryName(Path.GetDirectoryName(backup)), journal.target)
                && Path.GetFileName(Path.GetDirectoryName(backup)) == ".em-update-backup", "Некорректный путь резервной копии.");
            Require(journal.files.Select(f => f.path).Distinct(StringComparer.OrdinalIgnoreCase).Count() == journal.files.Length
                && journal.files.All(f => IsAllowedFile(f.path) || f.path == State), "Некорректный журнал восстановления.");
            NoLinks(journal.target);
            Require(!Same(Assembly.GetExecutingAssembly().Location, Path.Combine(journal.target, Agent)), "Для восстановления запустите копию EmUpdate.exe из другой папки, закрыв программу.");
            if (journal.pending != null)
            {
                Require(journal.files.Any(f => f.path == journal.pending), "Некорректная незавершенная операция.");
                journal.applied = journal.applied.Concat(new[] { journal.pending }).Distinct().ToArray();
            }
            using (var mutex = Acquire(journal.target))
            {
                CheckOtherInstances(journal.target);
                Rollback(backup, journal);
            }
        }

        internal static UpdateLock Acquire(string target)
        {
            string key;
            using (var sha = SHA256.Create()) key = BitConverter.ToString(sha.ComputeHash(Encoding.UTF8.GetBytes(Path.GetFullPath(target).ToUpperInvariant()))).Replace("-", "");
            var mutex = new Mutex(false, "Local\\EmToolsUpdate-" + key);
            bool acquired;
            try { acquired = mutex.WaitOne(0); }
            catch (AbandonedMutexException) { acquired = true; }
            if (!acquired) { mutex.Dispose(); throw new IOException("Обновление уже выполняется."); }
            return new UpdateLock(mutex);
        }

        internal static void CheckOtherInstances(string target)
        {
            foreach (var process in Process.GetProcessesByName(Path.GetFileNameWithoutExtension(App)))
                using (process)
                {
                    try { Require(!Same(process.MainModule.FileName, Path.Combine(target, App)), "Закройте все экземпляры программы перед установкой."); }
                    catch (System.ComponentModel.Win32Exception) { throw new IOException("Не удалось проверить запущенные экземпляры программы."); }
                    catch (InvalidOperationException) { }
                }
        }

        internal static void WaitForParent(InstallJob job)
        {
            if (job.parent_pid <= 0) return;
            try
            {
                using (Process parent = Process.GetProcessById(job.parent_pid))
                {
                    Require(parent.StartTime.ToUniversalTime().Ticks == job.parent_start_ticks
                        && Same(parent.MainModule.FileName, Path.Combine(job.target, App)), "Процесс приложения не соответствует заданию.");
                    Require(parent.WaitForExit(60000), "Программа не закрылась. Обновление отменено.");
                }
            }
            catch (ArgumentException) { }
        }

        public static string Apply(InstallJob job)
        {
            if (job != null && job.component == PythonRuntime.Component) return PythonRuntime.Apply(job);
            Require(job != null && job.component == "application" && !String.IsNullOrEmpty(job.target) && !String.IsNullOrEmpty(job.stage), "Некорректное задание установки.");
            job.target = Path.GetFullPath(job.target).TrimEnd('\\');
            NoLinks(job.target);
            NoLinks(job.stage);
            Require(Directory.Exists(job.target) && !Same(job.stage, job.target), "Некорректная папка установки.");
            Require(!Same(Assembly.GetExecutingAssembly().Location, Path.Combine(job.target, Agent)), "Для установки запустите копию EmUpdate.exe из другой папки.");
            WaitForParent(job);
            using (var mutex = Acquire(job.target))
            {
                CheckOtherInstances(job.target);
                Manifest m = ReadManifest(Path.Combine(job.stage, "manifest.json"));
                Require(m.build > CurrentBuild(job.target), "Установлена такая же или более новая сборка.");
                foreach (PayloadFile file in m.files) VerifyFile(Path.Combine(job.stage, file.path), file.size, file.sha256);
                string backupRoot = Path.Combine(job.target, ".em-update-backup");
                NoLinks(backupRoot);
                Directory.CreateDirectory(backupRoot);
                Require(PendingBackup(job.target) == null, "Найдена незавершенная установка. Сначала выполните восстановление.");
                string backup = Path.Combine(backupRoot, Guid.NewGuid().ToString("N"));
                Directory.CreateDirectory(backup);
                string validatedManifest = Path.Combine(backup, "installed-manifest.json");
                WriteJson(validatedManifest, m);
                var snapshots = new List<Snapshot>();
                foreach (string name in m.files.Select(f => f.path).Concat(new[] { State }))
                {
                    string path = Path.Combine(job.target, name);
                    NoLinks(path);
                    Require(!Directory.Exists(path), "Путь файла занят каталогом: " + name);
                    bool exists = File.Exists(path);
                    if (exists)
                    {
                        string snapshot = Path.Combine(backup, name);
                        Directory.CreateDirectory(Path.GetDirectoryName(snapshot));
                        File.Copy(path, snapshot, false);
                    }
                    snapshots.Add(new Snapshot { path = name, existed = exists, sha256 = exists ? Hash(Path.Combine(backup, name)) : null });
                }
                var journal = new Journal { target = job.target, status = "applying", files = snapshots.ToArray() };
                string journalPath = Path.Combine(backup, "transaction.json");
                WriteJson(journalPath, journal);
                try
                {
                    foreach (string name in m.files.Select(f => f.path).Concat(new[] { State }))
                    {
                        journal.pending = name;
                        WriteJson(journalPath, journal);
                        string destination = Path.Combine(job.target, name);
                        Directory.CreateDirectory(Path.GetDirectoryName(destination));
                        AtomicCopy(name == State ? validatedManifest : Path.Combine(job.stage, name), destination);
                        journal.applied = journal.applied.Concat(new[] { name }).ToArray();
                        if (name != State)
                        {
                            PayloadFile installed = m.files.Single(f => f.path == name);
                            VerifyFile(Path.Combine(job.target, name), installed.size, installed.sha256);
                        }
                        journal.pending = null;
                        WriteJson(journalPath, journal);
                    }
                    if (job.restart) Process.Start(new ProcessStartInfo(Path.Combine(job.target, App)) { WorkingDirectory = job.target, UseShellExecute = false });
                    journal.status = "installed";
                    WriteJson(journalPath, journal);
                    return backup;
                }
                catch (Exception installError)
                {
                    try { Rollback(backup, journal); }
                    catch (Exception rollbackError) { throw new IOException("Ошибка установки и отката. Резервная копия: " + backup + ". " + rollbackError.Message, installError); }
                    throw new IOException("Обновление не установлено. Предыдущие файлы восстановлены. Резервная копия: " + backup, installError);
                }
            }
        }
    }

    public sealed class UpdateSettings
    {
        public bool automatic;
        public string base_uri = "ftp://89.189.156.184/VORONEJ_FTP/UPDATE/ELECTROMAGNETIC_TOOLS_3.0/";
        public string username = "";
        public string protected_password = "";

        internal string Password()
        {
            return String.IsNullOrEmpty(protected_password) ? "" : Encoding.UTF8.GetString(ProtectedData.Unprotect(Convert.FromBase64String(protected_password), null, DataProtectionScope.CurrentUser));
        }

        internal void SetPassword(string value)
        {
            protected_password = String.IsNullOrEmpty(value) ? "" : Convert.ToBase64String(ProtectedData.Protect(Encoding.UTF8.GetBytes(value), null, DataProtectionScope.CurrentUser));
        }
    }

    public sealed class UpdateController
    {
        readonly Form owner;
        readonly ToolStripMenuItem check;
        readonly ToolStripMenuItem automatic;
        readonly ToolStripMenuItem python;
        readonly string settingsPath;
        readonly string target;
        UpdateSettings settings;
        bool busy;

        public static void Attach(Form owner, ToolStripMenuItem help)
        {
            new UpdateController(owner, help);
        }

        UpdateController(Form owner, ToolStripMenuItem help)
        {
            this.owner = owner;
            target = Application.StartupPath;
            string id;
            using (var sha = SHA256.Create()) id = BitConverter.ToString(sha.ComputeHash(Encoding.UTF8.GetBytes(target.ToUpperInvariant()))).Replace("-", "").Substring(0, 16);
            string directory = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData), "AMK_Gorizont", "ElectromagneticTools3", id);
            settingsPath = Path.Combine(directory, "updates.json");
            settings = File.Exists(settingsPath) ? LoadSettings() : new UpdateSettings();
            check = new ToolStripMenuItem("Проверить обновления...");
            check.Click += async (sender, e) => await Check(false);
            automatic = new ToolStripMenuItem("Проверять обновления при запуске") { CheckOnClick = true, Checked = settings.automatic };
            automatic.CheckedChanged += (sender, e) => { settings.automatic = automatic.Checked; SaveSettings(); };
            var options = new ToolStripMenuItem("Настройки FTP-обновления...");
            options.Click += (sender, e) => Configure();
            help.DropDownItems.Add(new ToolStripSeparator());
            help.DropDownItems.Add(check);
            help.DropDownItems.Add(automatic);
            help.DropDownItems.Add(options);
            python = new ToolStripMenuItem("Python: установить / обновить среду...");
            python.Click += async (sender, e) => await Check(false, true);
            help.DropDownItems.Add(python);
            var probePython = new ToolStripMenuItem("Python: проверить установленную среду...");
            probePython.Click += async (sender, e) =>
            {
                if (busy) return;
                busy = true;
                try
                {
                    var result = await Task.Run(() => PythonRuntime.Probe(Path.Combine(target, PythonRuntime.DirectoryName)));
                    if (!owner.IsDisposed) MessageBox.Show(owner, UpdateEngine.Json.Serialize(result), "Проверка Python");
                }
                catch (Exception ex) { if (!owner.IsDisposed) MessageBox.Show(owner, ex.Message, "Проверка Python", MessageBoxButtons.OK, MessageBoxIcon.Warning); }
                finally { busy = false; }
            };
            help.DropDownItems.Add(probePython);
            owner.Shown += async (sender, e) =>
            {
                try
                {
                    string pending = UpdateEngine.PendingBackup(target);
                    if (pending != null)
                    {
                        MessageBox.Show(owner, "Предыдущая установка была прервана. Сохранена резервная копия:\n" + pending
                            + "\n\nЗакройте программу и выполните восстановление командой EmUpdate.exe --recover, запустив копию updater из другой папки.", "Незавершенное обновление", MessageBoxButtons.OK, MessageBoxIcon.Warning);
                        return;
                    }
                }
                catch (Exception ex) { MessageBox.Show(owner, "Не удалось проверить журнал обновления: " + ex.Message); return; }
                if (settings.automatic) await Check(true);
            };
        }

        UpdateSettings LoadSettings()
        {
            try { return UpdateEngine.ReadJson<UpdateSettings>(settingsPath) ?? new UpdateSettings(); }
            catch { return new UpdateSettings(); }
        }

        bool SaveSettings()
        {
            try
            {
                Directory.CreateDirectory(Path.GetDirectoryName(settingsPath));
                UpdateEngine.WriteJson(settingsPath, settings);
                return true;
            }
            catch (Exception ex) { MessageBox.Show(owner, "Не удалось сохранить настройки обновлений: " + ex.Message); return false; }
        }

        void Configure()
        {
            using (var form = new Form { Text = "FTP-обновление", Size = new Size(620, 270), StartPosition = FormStartPosition.CenterParent, FormBorderStyle = FormBorderStyle.FixedDialog, MaximizeBox = false, MinimizeBox = false })
            {
                var uri = new TextBox { Left = 125, Top = 20, Width = 460, Text = settings.base_uri };
                var user = new TextBox { Left = 125, Top = 60, Width = 460, Text = settings.username };
                var password = new TextBox { Left = 125, Top = 100, Width = 460, UseSystemPasswordChar = true };
                try { password.Text = settings.Password(); }
                catch { MessageBox.Show(owner, "Сохраненный пароль недоступен в этой учетной записи Windows. Введите пароль заново."); }
                form.Controls.AddRange(new Control[] { new Label { Left = 12, Top = 23, Text = "Каталог FTP", Width = 110 }, uri,
                    new Label { Left = 12, Top = 63, Text = "Пользователь", Width = 110 }, user,
                    new Label { Left = 12, Top = 103, Text = "Пароль", Width = 110 }, password,
                    new Label { Left = 12, Top = 140, Width = 570, Height = 35, Text = "Пароль хранится с защитой Windows. FTP и SHA-256 не подтверждают подлинность пакета: цифровая подпись отключена." } });
                var save = new Button { Left = 470, Top = 185, Width = 115, Text = "Сохранить" };
                save.Click += (sender, e) =>
                {
                    Uri parsed;
                    if (!Uri.TryCreate(uri.Text.Trim(), UriKind.Absolute, out parsed) || parsed.Scheme != "ftp" || !String.IsNullOrEmpty(parsed.UserInfo)
                        || !String.IsNullOrEmpty(parsed.Query) || !String.IsNullOrEmpty(parsed.Fragment))
                    { MessageBox.Show(form, "Укажите FTP-каталог без логина и пароля в адресе."); return; }
                    try
                    {
                        var next = new UpdateSettings { base_uri = parsed.AbsoluteUri.TrimEnd('/') + "/", username = user.Text, automatic = settings.automatic };
                        next.SetPassword(password.Text);
                        settings = next;
                        if (SaveSettings()) form.Close();
                    }
                    catch (CryptographicException ex) { MessageBox.Show(form, "Не удалось защитить пароль средствами Windows: " + ex.Message); }
                };
                form.Controls.Add(save);
                form.AcceptButton = save;
                form.ShowDialog(owner);
            }
        }

        internal static FtpWebRequest CreateDownloadRequest(UpdateSettings settings, string name)
        {
            Uri root = new Uri(settings.base_uri);
            UpdateEngine.Require(root.Scheme == "ftp" && String.IsNullOrEmpty(root.UserInfo) && String.IsNullOrEmpty(root.Query) && String.IsNullOrEmpty(root.Fragment), "Некорректный адрес FTP.");
            var request = (FtpWebRequest)WebRequest.Create(new Uri(root, name));
            request.Method = WebRequestMethods.Ftp.DownloadFile;
            request.Proxy = null;
            request.Credentials = new NetworkCredential(settings.username, settings.Password());
            request.UseBinary = true;
            request.UsePassive = true;
            request.KeepAlive = false;
            request.CachePolicy = new System.Net.Cache.RequestCachePolicy(System.Net.Cache.RequestCacheLevel.NoCacheNoStore);
            request.Timeout = 15000;
            request.ReadWriteTimeout = 15000;
            return request;
        }

        internal static void Download(UpdateSettings settings, string name, string output, long limit, long expected)
        {
            var request = CreateDownloadRequest(settings, name);
            bool created = false;
            try
            {
                using (var response = (FtpWebResponse)request.GetResponse())
                using (var input = response.GetResponseStream())
                using (var destination = new FileStream(output, FileMode.CreateNew))
                {
                    created = true;
                    byte[] buffer = new byte[65536];
                    long total = 0;
                    int count;
                    var deadline = Stopwatch.StartNew();
                    while ((count = input.Read(buffer, 0, buffer.Length)) > 0)
                    {
                        total += count;
                        UpdateEngine.Require(total <= limit && deadline.Elapsed < TimeSpan.FromMinutes(10), "Превышен размер или время загрузки.");
                        destination.Write(buffer, 0, count);
                    }
                    UpdateEngine.Require(expected < 0 || total == expected, "Неполная загрузка пакета.");
                }
            }
            catch
            {
                if (created && File.Exists(output)) File.Delete(output);
                throw;
            }
        }

        async Task Check(bool silent, bool runtime = false)
        {
            if (busy) return;
            busy = true;
            check.Enabled = false;
            python.Enabled = false;
            check.Text = runtime ? "Проверка Python..." : "Проверка обновлений...";
            var channel = new UpdateSettings { base_uri = settings.base_uri, username = settings.username, protected_password = settings.protected_password };
            string download = Path.Combine(Path.GetTempPath(), "EmToolsDownload-" + Guid.NewGuid().ToString("N"));
            string stage = null;
            bool handedOff = false;
            try
            {
                UpdateEngine.Require(UpdateEngine.PendingBackup(target) == null, "Найдена незавершенная установка. Сначала выполните восстановление из резервной копии.");
                Directory.CreateDirectory(download);
                string manifestPath = Path.Combine(download, "manifest.json");
                Manifest manifest = await Task.Run(() => {
                    Download(channel, runtime ? "python-runtime.json" : "manifest.json", manifestPath, runtime ? PythonRuntime.ManifestLimit : 1024 * 1024, -1);
                    return runtime ? PythonRuntime.ReadManifest(manifestPath) : UpdateEngine.ReadManifest(manifestPath);
                });
                if (owner.IsDisposed || owner.Disposing) return;
                long installedBuild = runtime ? PythonRuntime.CurrentBuild(target) : UpdateEngine.CurrentBuild(target);
                if (manifest.build <= installedBuild)
                {
                    if (!silent) MessageBox.Show(owner, "Новых обновлений нет.", "Обновление ПО");
                    return;
                }
                string current = Assembly.GetEntryAssembly().GetName().Version.ToString();
                if (MessageBox.Show(owner, (runtime ? "Доступна среда Python для Electromagnetic Tools 3.0\nТекущая сборка среды: " + installedBuild : "Доступно обновление Electromagnetic Tools 3.0\nТекущая версия: " + current + " (сборка " + installedBuild + ")") + "\nНовая версия: "
                    + manifest.version + " (сборка " + manifest.build + ")\nРазмер ZIP: " + (manifest.size / 1048576.0).ToString("F1") + " МБ\nДата: " + manifest.released_utc + "\n\n" + manifest.notes
                    + "\n\nЦифровая подпись отключена. SHA-256 проверяет целостность, но не подлинность.\nСкачать обновление?", "Обновление ПО", MessageBoxButtons.YesNo, MessageBoxIcon.Question) != DialogResult.Yes) return;
                check.Text = "Загрузка обновления...";
                stage = await Task.Run(() => { string zip = Path.Combine(download, manifest.package); Download(channel, manifest.package, zip, UpdateEngine.MaxPackage, manifest.size); return runtime ? PythonRuntime.Prepare(manifestPath, zip, target) : UpdateEngine.Prepare(manifestPath, zip, target); });
                if (owner.IsDisposed || owner.Disposing) return;
                if (MessageBox.Show(owner, "Пакет проверен. Закрыть программу и установить обновление?\nНастройки и пользовательские данные сохранятся." + (runtime ? "\nБудет заменена только папка python_runtime с резервным копированием. Закройте работающие Python-скрипты. При установке будут запущены тесты Python и библиотек." : ""), "Установка обновления", MessageBoxButtons.YesNo, MessageBoxIcon.Question) != DialogResult.Yes) return;
                string worker = Path.Combine(download, UpdateEngine.Agent);
                File.Copy(Assembly.GetExecutingAssembly().Location, worker);
                var parent = Process.GetCurrentProcess();
                string jobPath = Path.Combine(download, "job.json");
                UpdateEngine.WriteJson(jobPath, new InstallJob { component = runtime ? PythonRuntime.Component : "application", stage = stage, target = target, parent_pid = parent.Id, parent_start_ticks = parent.StartTime.ToUniversalTime().Ticks, restart = true });
                Process.Start(new ProcessStartInfo(worker, "--apply-job \"" + jobPath + "\"") { WorkingDirectory = download, UseShellExecute = false, CreateNoWindow = true });
                handedOff = true;
                owner.Close();
            }
            catch (WebException ex)
            {
                if (!silent && !owner.IsDisposed) MessageBox.Show(owner, "Не удалось получить обновление с FTP (" + ex.Status + "). Проверьте доступ и настройки подключения.", "Обновление ПО");
            }
            catch (Exception ex)
            {
                if (!silent && !owner.IsDisposed) MessageBox.Show(owner, ex.Message, "Обновление ПО", MessageBoxButtons.OK, MessageBoxIcon.Warning);
            }
            finally
            {
                busy = false;
                if (!owner.IsDisposed) { check.Enabled = true; python.Enabled = true; check.Text = "Проверить обновления..."; }
                if (!handedOff)
                {
                    try { if (stage != null) Directory.Delete(stage, true); if (Directory.Exists(download)) Directory.Delete(download, true); }
                    catch (IOException) { }
                    catch (UnauthorizedAccessException) { }
                }
            }
        }
    }

    public static class AgentProgram
    {
        static void CleanupJob(string jobPath, InstallJob job)
        {
            try
            {
                bool runtime = job.component == PythonRuntime.Component;
                Manifest m = runtime ? PythonRuntime.ReadManifest(Path.Combine(job.stage, "manifest.json")) : UpdateEngine.ReadManifest(Path.Combine(job.stage, "manifest.json"));
                if (runtime) PythonRuntime.CleanupStage(job.stage, m);
                else if (UpdateEngine.Same(Path.GetDirectoryName(job.stage), Path.GetTempPath()) && Path.GetFileName(job.stage).StartsWith("EmToolsUpdate-", StringComparison.Ordinal))
                {
                    UpdateEngine.NoLinks(job.stage);
                    foreach (string name in m.files.Select(f => f.path).Concat(new[] { "manifest.json" }))
                    {
                        string path = Path.Combine(job.stage, name);
                        UpdateEngine.NoLinks(path);
                        if (File.Exists(path)) File.Delete(path);
                    }
                    Directory.Delete(job.stage, true);
                }
                string directory = Path.GetDirectoryName(Path.GetFullPath(jobPath));
                if (UpdateEngine.Same(directory, Path.GetDirectoryName(Assembly.GetExecutingAssembly().Location))
                    && UpdateEngine.Same(Path.GetDirectoryName(directory), Path.GetTempPath()) && Path.GetFileName(directory).StartsWith("EmToolsDownload-", StringComparison.Ordinal))
                {
                    foreach (string name in new[] { m.package, "manifest.json", "job.json" })
                    {
                        string path = Path.Combine(directory, name);
                        UpdateEngine.NoLinks(path);
                        if (File.Exists(path)) File.Delete(path);
                    }
                }
            }
            catch (IOException) { Console.WriteLine("[CLEANUP] Временные файлы оставлены для диагностики."); }
            catch (UnauthorizedAccessException) { Console.WriteLine("[CLEANUP] Временные файлы оставлены для диагностики."); }
        }

        [STAThread]
        public static int Main(string[] args)
        {
            Console.OutputEncoding = Encoding.UTF8;
            try
            {
                if (args.Length == 6 && args[0] == "--download")
                {
                    long limit = Int64.Parse(args[4], System.Globalization.CultureInfo.InvariantCulture);
                    long expected = Int64.Parse(args[5], System.Globalization.CultureInfo.InvariantCulture);
                    UpdateEngine.Require(args[2] == Path.GetFileName(args[2]) && limit > 0 && limit <= UpdateEngine.MaxPackage
                        && expected >= -1 && expected <= limit, "Некорректные параметры загрузки.");
                    UpdateController.Download(UpdateEngine.ReadJson<UpdateSettings>(args[1]), args[2], args[3], limit, expected);
                    Console.WriteLine("[DOWNLOAD SUCCESS]");
                    return 0;
                }
                if (args.Length == 2 && args[0] == "--python-probe")
                {
                    Console.WriteLine(UpdateEngine.Json.Serialize(PythonRuntime.Probe(args[1])));
                    return 0;
                }
                if (args.Length == 4 && args[0] == "--runtime-prepare")
                {
                    Console.WriteLine(PythonRuntime.Prepare(args[1], args[2], args[3]));
                    return 0;
                }
                if (args.Length == 4 && args[0] == "--runtime-install-local")
                {
                    string stage = PythonRuntime.Prepare(args[1], args[2], args[3]);
                    try { Console.WriteLine("[RUNTIME SUCCESS] Backup: " + PythonRuntime.Apply(new InstallJob { component = PythonRuntime.Component, stage = stage, target = args[3] })); }
                    finally { Directory.Delete(stage, true); }
                    return 0;
                }
                if (args.Length == 4 && args[0] == "--prepare")
                {
                    Console.WriteLine(UpdateEngine.Prepare(args[1], args[2], args[3]));
                    return 0;
                }
                if (args.Length == 2 && args[0] == "--info")
                {
                    Console.WriteLine(AssemblyName.GetAssemblyName(args[1]).Version.ToString());
                    return 0;
                }
                if (args.Length == 2 && args[0] == "--recover")
                {
                    UpdateEngine.Recover(args[1]);
                    Console.WriteLine("[RECOVERY SUCCESS]");
                    return 0;
                }
                if ((args.Length == 2 || args.Length == 3 && args[2] == "--quiet") && args[0] == "--apply-job")
                {
                    InstallJob job = UpdateEngine.ReadJson<InstallJob>(args[1]);
                    string backup = UpdateEngine.Apply(job);
                    CleanupJob(args[1], job);
                    Console.WriteLine("[UPDATE SUCCESS] Backup: " + backup);
                    return 0;
                }
                if (args.Length == 4 && args[0] == "--install-local")
                {
                    string stage = UpdateEngine.Prepare(args[1], args[2], args[3]);
                    try { Console.WriteLine("[UPDATE SUCCESS] Backup: " + UpdateEngine.Apply(new InstallJob { stage = stage, target = args[3], restart = false })); }
                    finally { Directory.Delete(stage, true); }
                    return 0;
                }
                Console.Error.WriteLine("Usage: --prepare manifest zip target | --install-local manifest zip target | --recover backup | --info app.exe");
                return 2;
            }
            catch (Exception ex)
            {
                Console.Error.WriteLine("[UPDATE FAILED] " + ex.Message);
                if (args.Length > 0 && args[0] == "--apply-job" && !args.Contains("--quiet")) MessageBox.Show(ex.Message, "Ошибка установки обновления", MessageBoxButtons.OK, MessageBoxIcon.Error);
                return 1;
            }
        }
    }
}
