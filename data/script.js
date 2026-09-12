document.addEventListener("DOMContentLoaded", () => {
  const body = document.body;
  const nav = document.getElementById("nav");
  const navScrim = document.getElementById("navScrim");
  const hamburgerButton = document.getElementById("hamburgerBtn");
  const navCloseButton = document.getElementById("navClose");
  const navLinks = document.querySelectorAll(".nav-link[data-target]");
  const views = document.querySelectorAll(".view[data-view]");
  const topbarTitle = document.getElementById("topbarTitle");
  const themeButtons = document.querySelectorAll(".theme-toggle");
  const wifiChip = document.getElementById("wifiChip");
  const wifiDot = document.getElementById("wifiDot");
  const wifiText = document.getElementById("wifiText");
  const wifiForm = document.getElementById("wifiForm");
  const wifiSsid = document.getElementById("wifiSsid");
  const wifiPass = document.getElementById("wifiPass");
  const wifiCurrentText = document.getElementById("wifiCurrentText");
  const wifiCurrentDot = document.getElementById("wifiCurrentDot");
  const scanWifiButton = document.getElementById("scanWifiBtn");
  const scanHint = document.getElementById("scanHint");
  const wifiConnectButton = document.getElementById("wifiConnectBtn");
  const wifiConnectedPanel = document.getElementById("wifiConnectedPanel");
  const connectedWifiName = document.getElementById("connectedWifiName");
  const connectedWifiIp = document.getElementById("connectedWifiIp");
  const changeWifiButton = document.getElementById("changeWifiBtn");
  const wifiConfigHint = document.getElementById("wifiConfigHint");
  const toggleWifiPassButton = document.getElementById("toggleWifiPass");
  const tempValue = document.getElementById("tempValue");
  const humValue = document.getElementById("humValue");
  const tempMeta = document.getElementById("tempMeta");
  const humMeta = document.getElementById("humMeta");
  const pumpSwitch = document.getElementById("pumpSwitch");
  const pumpStatusText = document.getElementById("pumpStatusText");
  const pumpIcon = document.getElementById("pumpIcon");
  let configurationOnly = false;

  const setTheme = (theme) => {
    document.documentElement.dataset.theme = theme;
    localStorage.setItem("aquactrl-theme", theme);
    themeButtons.forEach((button) => {
      const nextTheme = theme === "dark" ? "light" : "dark";
      button.setAttribute(
        "aria-label",
        nextTheme === "light" ? "Aktifkan mode terang" : "Aktifkan mode gelap",
      );
      button.setAttribute(
        "title",
        nextTheme === "light" ? "Aktifkan mode terang" : "Aktifkan mode gelap",
      );
    });
  };

  const savedTheme = localStorage.getItem("aquactrl-theme");
  if (savedTheme === "light" || savedTheme === "dark") {
    setTheme(savedTheme);
  }

  themeButtons.forEach((button) => {
    button.addEventListener("click", () => {
      const currentTheme = document.documentElement.dataset.theme || "dark";
      setTheme(currentTheme === "dark" ? "light" : "dark");
    });
  });

  const closeNavigation = () => {
    nav?.classList.remove("is-open");
    navScrim?.classList.remove("is-open");
    hamburgerButton?.setAttribute("aria-expanded", "false");
  };

  const showView = (target) => {
    if (configurationOnly && target !== "wifi") target = "wifi";
    const view =
      document.querySelector(`.view[data-view="${target}"]`) ||
      document.querySelector('.view[data-view="dashboard"]');
    if (!view) return;

    body.dataset.layout = "single";
    views.forEach((item) => item.classList.toggle("is-active", item === view));
    navLinks.forEach((link) => {
      link.classList.toggle("active", link.dataset.target === target);
    });

    if (topbarTitle) {
      topbarTitle.textContent =
        target === "mqtt"
          ? "Profil MQTT"
          : target === "modbus"
            ? "Modbus & Pin"
            : target === "wifi"
              ? "Konfigurasi WiFi"
              : "Dashboard";
    }

    history.replaceState(null, "", `#${target}`);
    closeNavigation();
  };

  hamburgerButton?.addEventListener("click", () => {
    const isOpen = nav?.classList.toggle("is-open") ?? false;
    navScrim?.classList.toggle("is-open", isOpen);
    hamburgerButton.setAttribute("aria-expanded", String(isOpen));
  });
  navCloseButton?.addEventListener("click", closeNavigation);
  navScrim?.addEventListener("click", closeNavigation);
  navLinks.forEach((link) => {
    link.addEventListener("click", (event) => {
      event.preventDefault();
      showView(link.dataset.target);
    });
  });
  document
    .getElementById("wifiBackBtn")
    ?.addEventListener("click", () => showView("dashboard"));
  window.addEventListener("hashchange", () =>
    showView(location.hash.slice(1) || "dashboard"),
  );

  const updateWifiStatus = (isConnected, ssid = "") => {
    if (isConnected) {
      wifiChip.classList.add("is-on");
      wifiDot.classList.add("is-on");
      wifiText.textContent = ssid || "Tersambung";
      wifiText.title = ssid || "Tersambung";
      if (wifiCurrentText)
        wifiCurrentText.textContent = `Tersambung ke ${ssid || "jaringan WiFi"}`;
      wifiCurrentDot?.classList.add("is-on");
    } else {
      wifiChip.classList.remove("is-on");
      wifiDot.classList.remove("is-on");
      wifiText.textContent = "Not connected";
      wifiText.title = "Belum tersambung";
      if (wifiCurrentText)
        wifiCurrentText.textContent = "Belum tersambung ke jaringan manapun";
      wifiCurrentDot?.classList.remove("is-on");
    }
  };

  const refreshWifiStatus = async () => {
    try {
      const response = await fetch("/api/status", { cache: "no-store" });
      if (!response.ok) throw new Error("Status WiFi tidak tersedia");
      const status = await response.json();
      if (tempValue) tempValue.textContent = status.temperature ?? "--";
      if (humValue) humValue.textContent = status.humidity ?? "--";
      if (tempMeta)
        tempMeta.textContent = status.sensorValid
          ? "Data sensor"
          : "Menunggu data sensor...";
      if (humMeta)
        humMeta.textContent = status.sensorValid
          ? "Data sensor"
          : "Menunggu data sensor...";
      if (pumpSwitch && typeof status.pump === "boolean") {
        pumpSwitch.setAttribute("aria-checked", String(status.pump));
        pumpStatusText.textContent = status.pump ? "Nyala" : "Mati";
        pumpIcon.classList.toggle("is-off", !status.pump);
      }
      configurationOnly = Boolean(status.configMode);
      updateWifiStatus(status.connected, status.connected ? status.ssid : "");
      if (wifiConnectedPanel) wifiConnectedPanel.hidden = configurationOnly;
      if (wifiForm) wifiForm.hidden = !configurationOnly;
      if (wifiConfigHint) wifiConfigHint.hidden = !configurationOnly;
      if (connectedWifiName)
        connectedWifiName.textContent = status.ssid || "Belum tersambung";
      if (connectedWifiIp)
        connectedWifiIp.textContent = `IP: ${status.ip || "-"}`;
      if (configurationOnly) {
        navLinks.forEach((link) => {
          link.hidden = link.dataset.target !== "wifi";
        });
        showView("wifi");
        if (wifiCurrentText)
          wifiCurrentText.textContent =
            "Terhubung ke ESP32-S3. Pilih WiFi tujuan di bawah.";
      }
    } catch {
      configurationOnly = false;
      if (wifiConnectedPanel) wifiConnectedPanel.hidden = false;
      if (wifiForm) wifiForm.hidden = true;
      if (wifiConfigHint) wifiConfigHint.hidden = true;
      updateWifiStatus(false);
    }
  };

  pumpSwitch?.addEventListener("click", async () => {
    const enabled = pumpSwitch.getAttribute("aria-checked") !== "true";
    pumpSwitch.disabled = true;
    try {
      const response = await fetch("/api/pump", {
        method: "POST",
        headers: { "Content-Type": "application/x-www-form-urlencoded" },
        body: new URLSearchParams({ state: enabled ? "on" : "off" }),
      });
      const result = await response.json();
      if (!result.ok) throw new Error(result.error || "Kontrol pompa gagal");
      pumpSwitch.setAttribute("aria-checked", String(result.pump));
      pumpStatusText.textContent = result.pump ? "Nyala" : "Mati";
      pumpIcon?.classList.toggle("is-off", !result.pump);
    } catch {
      refreshWifiStatus();
    } finally {
      pumpSwitch.disabled = false;
    }
  });

  toggleWifiPassButton?.addEventListener("click", () => {
    if (!wifiPass) return;
    const isPassword = wifiPass.type === "password";
    wifiPass.type = isPassword ? "text" : "password";
    toggleWifiPassButton.setAttribute(
      "aria-label",
      isPassword ? "Sembunyikan kata sandi" : "Tampilkan kata sandi",
    );
    const icon = toggleWifiPassButton.querySelector("use");
    icon?.setAttribute("href", isPassword ? "#icon-eye-off" : "#icon-eye");
  });

  changeWifiButton?.addEventListener("click", async () => {
    changeWifiButton.disabled = true;
    if (wifiConfigHint) {
      wifiConfigHint.hidden = false;
      wifiConfigHint.textContent =
        "ESP32 sedang restart ke mode konfigurasi. Tunggu WiFi ESP32-S3 muncul, lalu sambungkan perangkat ke sana.";
    }
    try {
      await fetch("/api/wifi/configure", { method: "POST" });
    } catch {
      // Koneksi terputus adalah normal karena ESP32 sedang restart.
    }
  });

  scanWifiButton?.addEventListener("click", async () => {
    if (scanHint) scanHint.textContent = "Memindai jaringan di sekitar...";
    scanWifiButton.disabled = true;
    try {
      const response = await fetch("/api/wifi/scan", { cache: "no-store" });
      const networks = await response.json();
      const list = document.getElementById("ssidList");
      if (list) {
        list.replaceChildren(
          ...networks
            .filter((network) => network.ssid)
            .map((network) => {
              const option = document.createElement("option");
              option.value = network.ssid;
              return option;
            }),
        );
      }
      if (scanHint)
        scanHint.textContent = `${networks.length} jaringan ditemukan.`;
    } catch {
      if (scanHint)
        scanHint.textContent =
          "Gagal memindai jaringan. Isi SSID secara manual.";
    } finally {
      scanWifiButton.disabled = false;
    }
  });

  wifiForm?.addEventListener("submit", async (event) => {
    event.preventDefault();
    if (!wifiSsid?.value.trim()) return;
    wifiConnectButton.disabled = true;
    if (wifiCurrentText)
      wifiCurrentText.textContent = "Menguji koneksi WiFi...";
    try {
      const response = await fetch("/api/wifi/connect", {
        method: "POST",
        headers: { "Content-Type": "application/x-www-form-urlencoded" },
        body: new URLSearchParams({
          ssid: wifiSsid.value.trim(),
          password: wifiPass?.value || "",
        }),
      });
      const result = await response.json();
      if (!result.ok) throw new Error(result.error || "Koneksi gagal");
      if (wifiCurrentText)
        wifiCurrentText.textContent = "Berhasil. ESP32 sedang restart...";
      setTimeout(() => window.location.reload(), 3000);
    } catch (error) {
      if (wifiCurrentText)
        wifiCurrentText.textContent = error.message || "Koneksi WiFi gagal.";
      wifiConnectButton.disabled = false;
    }
  });

  refreshWifiStatus();
  setInterval(refreshWifiStatus, 5000);

  const modalScrim = document.getElementById("modalScrim");
  const addProfileButton = document.getElementById("addProfileBtn");
  const closeModalButton = document.getElementById("modalCloseBtn");
  const cancelModalButton = document.getElementById("modalCancelBtn");
  const profileForm = document.getElementById("profileForm");

  if (
    !modalScrim ||
    !addProfileButton ||
    !closeModalButton ||
    !cancelModalButton ||
    !profileForm
  )
    return;

  const closeModal = () => {
    modalScrim.hidden = true;
  };

  addProfileButton.addEventListener("click", () => {
    modalScrim.hidden = false;
  });

  closeModalButton.addEventListener("click", closeModal);
  cancelModalButton.addEventListener("click", closeModal);

  modalScrim.addEventListener("click", (event) => {
    if (event.target === modalScrim) {
      closeModal();
    }
  });

  profileForm.addEventListener("submit", (event) => {
    event.preventDefault();
    closeModal();
  });

  document.addEventListener("keydown", (event) => {
    if (event.key === "Escape" && !modalScrim.hidden) {
      closeModal();
    }
  });
});
