const API = "";
const APP_WIDTH = 365;
const LOGIN_WIDTH = 360;
const LOGIN_HEIGHT_BUFFER = 10;
const APP_TARGET_HEIGHT = 720;
const WINDOW_HEIGHT_BUFFER = 40;

function measureNaturalHeight(screenId) {
  const screen = document.getElementById(screenId);
  const receipt = screen?.querySelector(".receipt");
  if (!screen || !receipt) {
    return 0;
  }

  document.body.classList.add("measuring");
  const height = Math.ceil(
    Math.max(receipt.scrollHeight, receipt.offsetHeight, receipt.getBoundingClientRect().height)
  );
  document.body.classList.remove("measuring");
  return height;
}

function updateAppExpandedLayout() {
  if (document.body.classList.contains("login-active")) {
    document.body.classList.remove("app-expanded");
    return;
  }

  const natural = measureNaturalHeight("app-screen");
  const expanded = window.innerHeight > natural + 24;
  document.body.classList.toggle("app-expanded", expanded);
}

function scheduleAppWindowFit() {
  window.requestAnimationFrame(() => {
    window.requestAnimationFrame(() => {
      syncNativeWindowSize({ fit: true });
      window.setTimeout(() => {
        syncNativeWindowSize({ fit: true });
        updateAppExpandedLayout();
        resetWebViewScrollPosition();
      }, 50);
    });
  });
}

function resetWebViewScrollPosition() {
  window.scrollTo(0, 0);
  document.documentElement.scrollTop = 0;
  document.body.scrollTop = 0;
}

async function api(path, options = {}) {
  const res = await fetch(API + path, {
    headers: { "Content-Type": "application/json" },
    ...options,
  });
  const data = await res.json();
  if (!res.ok) throw new Error(data.error || "Request failed");
  return data;
}

function hasNativeBridge() {
  return !!(
    window.webkit?.messageHandlers?.native ||
    window.chrome?.webview?.postMessage
  );
}

function postNative(message) {
  if (window.webkit?.messageHandlers?.native) {
    window.webkit.messageHandlers.native.postMessage(message);
    return true;
  }
  if (window.chrome?.webview?.postMessage) {
    window.chrome.webview.postMessage(message);
    return true;
  }
  return false;
}

function syncNativeWindowSize(options = {}) {
  const fit = options.fit === true;
  if (!hasNativeBridge()) {
    return;
  }

  window.scrollTo(0, 0);
  document.documentElement.scrollTop = 0;
  document.body.scrollTop = 0;

  const isLogin = document.body.classList.contains("login-active");
  const screenId = isLogin ? "login-screen" : "app-screen";
  const width = isLogin ? LOGIN_WIDTH : APP_WIDTH;
  const buffer = isLogin ? LOGIN_HEIGHT_BUFFER : WINDOW_HEIGHT_BUFFER;
  const measuredHeight = measureNaturalHeight(screenId);
  const height = isLogin
    ? measuredHeight + buffer
    : Math.max(measuredHeight + buffer, APP_TARGET_HEIGHT);
  const message = fit ? `resize:${width},${height},fit` : `resize:${width},${height}`;

  postNative(message);
}

window.syncNativeWindowSize = syncNativeWindowSize;

function setViewport(mode) {
  const isLogin = mode === "login";
  document.documentElement.classList.toggle("login-active", isLogin);
  document.body.classList.toggle("login-active", isLogin);
  window.requestAnimationFrame(() => {
    window.requestAnimationFrame(() => {
      syncNativeWindowSize({ fit: true });
    });
  });
}

function showApp() {
  document.getElementById("login-screen").classList.add("hidden");
  document.getElementById("app-screen").classList.remove("hidden");
  document.documentElement.classList.remove("login-active");
  document.body.classList.remove("login-active");
  document.body.classList.remove("app-expanded");
}

function folderActionLabel() {
  const platform = navigator.platform || "";
  if (/Mac|iPhone|iPad|iPod/i.test(platform)) {
    return "Open in Finder";
  }
  if (/Win/i.test(platform)) {
    return "Open in Explorer";
  }
  return "Open folder";
}

function basename(path) {
  const parts = path.split(/[/\\]/);
  return parts[parts.length - 1] || path;
}

function parseLine(entry) {
  const isDeposit = entry.startsWith("Deposit:");
  const isWithdraw = entry.startsWith("Withdraw:");
  const amountMatch = entry.match(/\$[\d.]+/);
  return {
    label: isDeposit ? "Deposit" : isWithdraw ? "Withdraw" : "Entry",
    detail: entry,
    amount: amountMatch ? amountMatch[0] : "",
    credit: isDeposit,
  };
}

function renderAccount(data, options = {}) {
  const balanceEl = document.getElementById("balance");
  const previousBalance = balanceEl.textContent;

  document.getElementById("account-title").textContent = "Account Display for " + data.name + ":";
  balanceEl.textContent = "$" + Number(data.balance).toFixed(2);

  const filePathEl = document.getElementById("file-path");
  const filePathText = filePathEl?.querySelector(".file-path-text");
  const filePathAction = filePathEl?.querySelector(".file-path-action");
  if (filePathEl && filePathText && data.path) {
    filePathEl.classList.remove("hidden");
    filePathEl.dataset.path = data.path;
    filePathText.textContent = basename(data.path);
    filePathText.title = data.path;
    if (filePathAction) {
      filePathAction.textContent = folderActionLabel();
    }
  } else if (filePathEl) {
    filePathEl.classList.add("hidden");
    delete filePathEl.dataset.path;
  }

  if (previousBalance !== balanceEl.textContent) {
    balanceEl.classList.remove("balance-updated");
    void balanceEl.offsetWidth;
    balanceEl.classList.add("balance-updated");
  }

  const historyEl = document.getElementById("history");
  historyEl.innerHTML = "";

  if (!data.history || data.history.length === 0) {
    historyEl.innerHTML = '<li class="empty">No transactions on record</li>';
    return;
  }

  const entries = [...data.history].reverse();

  for (const entry of entries) {
    const line = parseLine(entry);
    const li = document.createElement("li");
    li.innerHTML = `
      <div>
        <div>${line.label}</div>
        <div class="desc">${line.detail}</div>
      </div>
      <span class="amt ${line.credit ? "credit" : "debit"}">${line.amount}</span>
    `;
    historyEl.appendChild(li);
  }

  const newestItem = historyEl.firstElementChild;
  if (options.highlightNew && newestItem) {
    newestItem.classList.add("history-new");
    window.setTimeout(() => newestItem.classList.remove("history-new"), 1400);
  }

  if (options.scrollHistory !== false) {
    window.requestAnimationFrame(() => {
      historyEl.scrollTop = 0;
    });
  }
}

async function refresh() {
  const data = await api("/api/account");
  renderAccount(data);
}

async function doLogin() {
  const nameInput = document.getElementById("login-name");
  const name = nameInput.value.trim();
  const dir = document.getElementById("login-dir").value.trim();
  const loginStatus = document.getElementById("login-status");
  if (!name) {
    loginStatus.textContent = "Name is required";
    loginStatus.className = "status err";
    nameInput.focus();
    return;
  }
  try {
    const data = await api("/api/open", {
      method: "POST",
      body: JSON.stringify({ name, dir }),
    });
    showApp();
    renderAccount(data);
    scheduleAppWindowFit();
    focusAmountField();
  } catch (e) {
    loginStatus.textContent = e.message;
    loginStatus.className = "status err";
  }
}

document.getElementById("login-btn").addEventListener("click", doLogin);

document.getElementById("login-name").addEventListener("keydown", (e) => {
  if (e.key === "Enter") {
    doLogin();
  }
});

document.getElementById("login-dir").addEventListener("keydown", (e) => {
  if (e.key === "Enter") {
    doLogin();
  }
});

function updateLoginDirPreview() {
  const dirInput = document.getElementById("login-dir");
  const preview = document.getElementById("login-dir-preview");
  if (!dirInput || !preview) {
    return;
  }
  const raw = dirInput.value.trim();
  preview.textContent = raw ? basename(raw) : "Home folder";
  preview.title = raw || "Uses your Home folder when blank";
}

window.__setLoginDir = function (path) {
  const dirInput = document.getElementById("login-dir");
  if (!dirInput || typeof path !== "string") {
    return;
  }
  dirInput.value = path;
  updateLoginDirPreview();
};

document.getElementById("login-dir").addEventListener("input", updateLoginDirPreview);

document.getElementById("login-pick-dir").addEventListener("click", () => {
  const dir = document.getElementById("login-dir").value.trim();
  if (hasNativeBridge()) {
    postNative(`pickDir:${dir}`);
    return;
  }
  document.getElementById("login-dir").focus();
  alert("Type a folder path above, or run the native app to use the folder picker.");
});

document.getElementById("login-open-dir").addEventListener("click", async () => {
  const dir = document.getElementById("login-dir").value.trim();
  try {
    if (hasNativeBridge()) {
      postNative(`openDir:${dir}`);
      return;
    }
    await api("/api/open-dir", {
      method: "POST",
      body: JSON.stringify({ dir }),
    });
  } catch (e) {
    alert(e.message);
  }
});

document.getElementById("file-path").addEventListener("click", async () => {
  const filePathEl = document.getElementById("file-path");
  const path = filePathEl?.dataset.path;
  if (!path) {
    return;
  }
  try {
    if (hasNativeBridge()) {
      postNative(`openFile:${path}`);
      return;
    }
    await api("/api/open-file", { method: "POST", body: "{}" });
  } catch (e) {
    alert(e.message);
  }
});

window.addEventListener("load", () => {
  setViewport("login");
  updateLoginDirPreview();
  const openDirLabel = document.getElementById("login-open-dir");
  if (openDirLabel) {
    openDirLabel.textContent = folderActionLabel();
  }
  const nameInput = document.getElementById("login-name");
  if (!nameInput.value.trim()) {
    nameInput.value = "exampleAccount";
  }
  nameInput.focus();
  nameInput.select();
});

window.addEventListener("resize", () => {
  if (!document.body.classList.contains("login-active")) {
    updateAppExpandedLayout();
  }
});

function parseAmountInput() {
  const raw = document.getElementById("amount").value.trim();
  if (!raw) {
    return null;
  }
  const amount = Number(raw);
  if (!Number.isFinite(amount) || amount <= 0) {
    return null;
  }
  return amount;
}

async function doDeposit() {
  const amount = parseAmountInput();
  if (amount === null) {
    alert("Enter a valid amount greater than zero.");
    document.getElementById("amount").focus();
    return;
  }
  const depositBtn = document.getElementById("deposit-btn");
  depositBtn.disabled = true;
  try {
    const data = await api("/api/deposit", {
      method: "POST",
      body: JSON.stringify({ amount }),
    });
    document.getElementById("amount").value = "";
    renderAccount(data, { highlightNew: true, scrollHistory: true });
    focusAmountField();
  } catch (e) {
    alert(e.message);
  } finally {
    depositBtn.disabled = false;
  }
}

async function doWithdraw() {
  const amount = parseAmountInput();
  if (amount === null) {
    alert("Enter a valid amount greater than zero.");
    document.getElementById("amount").focus();
    return;
  }
  const withdrawBtn = document.getElementById("withdraw-btn");
  withdrawBtn.disabled = true;
  try {
    const data = await api("/api/withdraw", {
      method: "POST",
      body: JSON.stringify({ amount }),
    });
    document.getElementById("amount").value = "";
    renderAccount(data, { highlightNew: true, scrollHistory: true });
    focusAmountField();
  } catch (e) {
    alert(e.message);
  } finally {
    withdrawBtn.disabled = false;
  }
}

document.getElementById("deposit-btn").addEventListener("click", doDeposit);
document.getElementById("withdraw-btn").addEventListener("click", doWithdraw);

document.getElementById("amount").addEventListener("keydown", (e) => {
  if (e.key === "Enter") {
    doDeposit();
  }
});

function focusAmountField() {
  const amount = document.getElementById("amount");
  if (!amount || amount.closest(".hidden")) {
    return;
  }
  amount.focus();
}

document.getElementById("save-exit-btn").addEventListener("click", async () => {
  try {
    await api("/api/save", { method: "POST", body: "{}" });
    await api("/api/quit", { method: "POST", body: "{}" });
    if (postNative("quit")) {
      return;
    }
    window.close();
  } catch (e) {
    alert(e.message);
  }
});
