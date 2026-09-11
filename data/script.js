document.addEventListener("DOMContentLoaded", () => {
  const body = document.body;
  const nav = document.getElementById("nav");
  const navScrim = document.getElementById("navScrim");
  const hamburgerButton = document.getElementById("hamburgerBtn");
  const navCloseButton = document.getElementById("navClose");
  const navLinks = document.querySelectorAll(".nav-link[data-target]");
  const views = document.querySelectorAll(".view[data-view]");
  const topbarTitle = document.getElementById("topbarTitle");

  const closeNavigation = () => {
    nav?.classList.remove("is-open");
    navScrim?.classList.remove("is-open");
    hamburgerButton?.setAttribute("aria-expanded", "false");
  };

  const showView = (target) => {
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

  showView(location.hash.slice(1) || "dashboard");

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
