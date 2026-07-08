const API = "";

async function api(path, options = {}) {
  const res = await fetch(API + path, {
    headers: { "Content-Type": "application/json" },
    ...options,
  });
  const data = await res.json();
  if (!res.ok) throw new Error(data.error || "Request failed");
  return data;
}

function setViewport(mode) {
  const isLogin = mode === "login";
  document.documentElement.classList.toggle("login-active", isLogin);
  document.body.classList.toggle("login-active", isLogin);
  if (window.webkit?.messageHandlers?.native) {
    window.webkit.messageHandlers.native.postMessage(
      isLogin ? "resize:360,190" : "resize:365,880"
    );
  }
}

function showApp() {
  document.getElementById("login-screen").classList.add("hidden");
  document.getElementById("app-screen").classList.remove("hidden");
  setViewport("app");
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
  document.getElementById("subtitle").textContent = data.loaded
    ? "File Found... Loaded Existing Account Information."
    : "No Existing Information found. Creating New Account...";

  const filePathEl = document.getElementById("file-path");
  if (filePathEl && data.path) {
    filePathEl.textContent = "File: " + data.path;
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

window.addEventListener("load", () => {
  setViewport("login");
  const nameInput = document.getElementById("login-name");
  if (!nameInput.value.trim()) {
    nameInput.value = "exampleAccount";
  }
  nameInput.focus();
  nameInput.select();
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
    if (window.webkit?.messageHandlers?.native) {
      window.webkit.messageHandlers.native.postMessage("quit");
    } else {
      window.close();
    }
  } catch (e) {
    alert(e.message);
  }
});
