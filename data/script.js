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
  const mqttChip = document.getElementById("mqttChip");
  const mqttDot = document.getElementById("mqttDot");
  const mqttText = document.getElementById("mqttText");
  const profileList = document.getElementById("profileList");
  const profileEmpty = document.getElementById("profileEmpty");
  const mqttLimitHint = document.getElementById("mqttLimitHint");
  const modalTitle = document.getElementById("modalTitle");
  const modalSubmitButton = document.getElementById("modalSubmitBtn");
  let editingProfileId = -1;
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

  const updateMqttStatus = (connected, profileName = "", error = "") => {
    mqttChip?.classList.toggle("is-on", connected);
    mqttDot?.classList.toggle("is-on", connected);
    if (mqttText)
      mqttText.textContent = connected
        ? profileName || "Tersambung"
        : "Not connected";
    if (mqttText)
      mqttText.title = connected ? profileName : error || "Belum tersambung";
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
      updateMqttStatus(
        status.mqttConnected,
        status.mqttProfile,
        status.mqttError,
      );
      await loadMqttProfiles();
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

  let profileErrors = {};
  const profileError = (id) => profileErrors[id] || "";
  const profileAction = (label, action, id, className) => {
    const button = document.createElement("button");
    button.type = "button";
    button.className = `btn btn-sm ${className}`;
    button.dataset.action = action;
    button.dataset.id = id;
    button.textContent = label;
    return button;
  };

  const loadMqttProfiles = async () => {
    if (!profileList) return;
    try {
      const response = await fetch("/api/mqtt/profiles", { cache: "no-store" });
      const profiles = await response.json();
      profileList.replaceChildren();
      if (profileEmpty) profileEmpty.hidden = profiles.length > 0;
      if (mqttLimitHint) mqttLimitHint.hidden = profiles.length < 5;
      profiles.forEach((profile) => {
        const card = document.createElement("article");
        card.className = `profile-card${profile.active ? " is-active" : ""}`;
        card.dataset.profile = JSON.stringify(profile);
        const main = document.createElement("div");
        main.className = "profile-main";
        const row = document.createElement("div");
        row.className = "profile-name-row";
        const name = document.createElement("span");
        name.className = "profile-name";
        name.textContent = profile.name;
        row.append(name);
        if (profile.active) {
          const badge = document.createElement("span");
          badge.className = "badge";
          badge.textContent = "Aktif";
          row.append(badge);
        }
        const broker = document.createElement("div");
        broker.className = "profile-broker";
        broker.textContent = `${profile.broker}:${profile.port}${profile.tls ? " · TLS" : ""}`;
        main.append(row, broker);
        const actions = document.createElement("div");
        actions.className = "profile-actions";
        if (!profile.active)
          actions.append(
            profileAction("Aktifkan", "activate", profile.id, "btn-primary"),
          );
        actions.append(profileAction("Edit", "edit", profile.id, "btn-ghost"));
        actions.append(
          profileAction("Hapus", "delete", profile.id, "btn-danger-ghost"),
        );
        card.append(main, actions);
        if (profileError(profile.id)) {
          const error = document.createElement("p");
          error.className = "profile-error";
          error.textContent = profileError(profile.id);
          card.append(error);
        }
        profileList.append(card);
      });
    } catch {
      if (profileEmpty) profileEmpty.hidden = false;
    }
  };

  profileList?.addEventListener("click", async (event) => {
    const button = event.target.closest("button[data-action]");
    if (!button) return;
    const card = button.closest("[data-profile]");
    const profile = card ? JSON.parse(card.dataset.profile) : null;
    const id = Number(button.dataset.id);
    if (button.dataset.action === "edit" && profile) {
      editingProfileId = id;
      modalTitle.textContent = "Edit profil MQTT";
      modalSubmitButton.textContent = "Simpan perubahan";
      document.getElementById("profName").value = profile.name;
      document.getElementById("profBroker").value = profile.broker;
      document.getElementById("profPort").value = profile.port;
      document.getElementById("profTls").checked = profile.tls;
      document.getElementById("profUser").value = profile.username || "";
      document.getElementById("profPass").value = "";
      modalScrim.hidden = false;
      return;
    }

    if (button.dataset.action === "delete" && profile) {
      deletingProfileId = id;
      if (deleteModalMessage) {
        deleteModalMessage.textContent = `Apakah Anda yakin ingin menghapus profil "${profile.name}"?`;
      }
      if (deleteModalScrim) deleteModalScrim.hidden = false;
      return;
    }

    if (button.dataset.action === "activate") {
      button.disabled = true;
      button.classList.add("is-loading");
      const originalText = button.textContent;
      button.textContent = "Connecting...";
      try {
        const response = await fetch("/api/mqtt/activate", {
          method: "POST",
          headers: { "Content-Type": "application/x-www-form-urlencoded" },
          body: new URLSearchParams({ id: String(id) }),
        });
        const result = await response.json();
        if (!result.ok) {
          profileErrors[id] = result.error || "Broker tidak dapat dihubungkan.";
        } else {
          delete profileErrors[id];
        }
      } catch (err) {
        profileErrors[id] = "Gagal menghubungi ESP32.";
      } finally {
        await loadMqttProfiles();
        await refreshWifiStatus();
      }
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

  changeWifiButton?.addEventListener("click", () => {
    const wifiModalScrim = document.getElementById("wifiModalScrim");
    const wifiModalConfirmBody = document.getElementById(
      "wifiModalConfirmBody",
    );
    const wifiModalLoadingBody = document.getElementById(
      "wifiModalLoadingBody",
    );
    const wifiModalCloseBtn = document.getElementById("wifiModalCloseBtn");
    const wifiModalCancelBtn = document.getElementById("wifiModalCancelBtn");
    const wifiModalConfirmBtn = document.getElementById("wifiModalConfirmBtn");

    if (!wifiModalScrim) return;

    if (wifiModalConfirmBody) wifiModalConfirmBody.hidden = false;
    if (wifiModalLoadingBody) wifiModalLoadingBody.hidden = true;
    wifiModalScrim.hidden = false;

    const closeWifiModal = () => {
      wifiModalScrim.hidden = true;
    };

    wifiModalCloseBtn?.addEventListener("click", closeWifiModal, {
      once: true,
    });
    wifiModalCancelBtn?.addEventListener("click", closeWifiModal, {
      once: true,
    });

    wifiModalConfirmBtn?.addEventListener(
      "click",
      async () => {
        if (wifiModalConfirmBtn.disabled) return;
        wifiModalConfirmBtn.disabled = true;
        wifiModalConfirmBtn.classList.add("is-loading");

        if (wifiModalConfirmBody) wifiModalConfirmBody.hidden = true;
        if (wifiModalLoadingBody) wifiModalLoadingBody.hidden = false;

        try {
          await fetch("/api/wifi/configure", { method: "POST" });
        } catch {
          // Normal jika koneksi terputus karena ESP32 restart
        }
      },
      { once: true },
    );
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

  const deleteModalScrim = document.getElementById("deleteModalScrim");
  const deleteModalCloseBtn = document.getElementById("deleteModalCloseBtn");
  const deleteModalCancelBtn = document.getElementById("deleteModalCancelBtn");
  const deleteModalConfirmBtn = document.getElementById(
    "deleteModalConfirmBtn",
  );
  const deleteModalMessage = document.getElementById("deleteModalMessage");
  let deletingProfileId = -1;

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

  const closeDeleteModal = () => {
    if (deleteModalScrim) deleteModalScrim.hidden = true;
    deletingProfileId = -1;
  };

  addProfileButton.addEventListener("click", () => {
    editingProfileId = -1;
    modalTitle.textContent = "Tambah profil MQTT";
    modalSubmitButton.textContent = "Tambah profil";
    profileForm.reset();
    modalScrim.hidden = false;
  });

  closeModalButton.addEventListener("click", closeModal);
  cancelModalButton.addEventListener("click", closeModal);

  modalScrim.addEventListener("click", (event) => {
    if (event.target === modalScrim) {
      closeModal();
    }
  });

  deleteModalCloseBtn?.addEventListener("click", closeDeleteModal);
  deleteModalCancelBtn?.addEventListener("click", closeDeleteModal);
  deleteModalScrim?.addEventListener("click", (event) => {
    if (event.target === deleteModalScrim) {
      closeDeleteModal();
    }
  });

  deleteModalConfirmBtn?.addEventListener("click", async () => {
    if (deletingProfileId < 0) return;
    deleteModalConfirmBtn.disabled = true;
    deleteModalConfirmBtn.classList.add("is-loading");
    try {
      const response = await fetch("/api/mqtt/delete", {
        method: "POST",
        headers: { "Content-Type": "application/x-www-form-urlencoded" },
        body: new URLSearchParams({ id: String(deletingProfileId) }),
      });
      const result = await response.json();
      if (result.ok) {
        delete profileErrors[deletingProfileId];
      }
    } catch {
      // API error
    } finally {
      deleteModalConfirmBtn.disabled = false;
      deleteModalConfirmBtn.classList.remove("is-loading");
      closeDeleteModal();
      await loadMqttProfiles();
      await refreshWifiStatus();
    }
  });

  profileForm.addEventListener("submit", (event) => {
    event.preventDefault();
    if (modalSubmitButton) {
      modalSubmitButton.disabled = true;
      modalSubmitButton.classList.add("is-loading");
    }
    const profile = {
      id: String(editingProfileId),
      name: document.getElementById("profName").value.trim(),
      broker: document.getElementById("profBroker").value.trim(),
      port: document.getElementById("profPort").value,
      tls: String(document.getElementById("profTls").checked),
      username: document.getElementById("profUser").value.trim(),
      password: document.getElementById("profPass").value,
    };
    fetch("/api/mqtt/profile", {
      method: "POST",
      headers: { "Content-Type": "application/x-www-form-urlencoded" },
      body: new URLSearchParams(profile),
    })
      .then(async (response) => {
        const result = await response.json();
        if (!result.ok)
          throw new Error(result.error || "Profil gagal disimpan");
        closeModal();
        await loadMqttProfiles();
      })
      .catch((error) => {
        profileErrors[editingProfileId] = error.message;
        loadMqttProfiles();
      })
      .finally(() => {
        if (modalSubmitButton) {
          modalSubmitButton.disabled = false;
          modalSubmitButton.classList.remove("is-loading");
        }
      });
  });

  loadMqttProfiles();

  document.addEventListener("keydown", (event) => {
    if (event.key === "Escape") {
      if (!modalScrim.hidden) closeModal();
      if (deleteModalScrim && !deleteModalScrim.hidden) closeDeleteModal();
    }
  });
});
