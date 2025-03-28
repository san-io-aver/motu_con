// 🔥 Firebase Configuration
const firebaseConfig = {
    apiKey: "AIzaSyBNAtLfvBeAIBz2dhDeOlGKakw-CCvCKss",
    authDomain: "motu-connect.firebaseapp.com",
    databaseURL: "https://motu-connect-default-rtdb.firebaseio.com",
    projectId: "motu-connect",
    storageBucket: "motu-connect.firebasestorage.app",
    messagingSenderId: "828209943780",
    appId: "1:828209943780:web:739aab34b299595ac288f1"
};

// ✅ Initialize Firebase
firebase.initializeApp(firebaseConfig);
const db = firebase.database();

// ✅ Authenticate Anonymously & Enable Message Sending Only After Auth
firebase.auth().signInAnonymously()
  .then((userCredential) => {
    console.log("Signed in anonymously:", userCredential.user.uid);
    setupSendButton(); // ✅ Call setupSendButton here
  })
  .catch((error) => {
    console.error("Anonymous sign-in failed:", error.message);
  });

// ✅ Define setupSendButton function below
function setupSendButton() {
    let sendButton = document.getElementById("sendButton");
    if (!sendButton.dataset.listener) {
        sendButton.addEventListener("click", sendMessage);
        sendButton.dataset.listener = "true"; // Prevent multiple event listeners
    }
}


// 📩 Send Message to Firebase (Stores only the latest message)
function sendMessage() {
    let msg = document.getElementById("messageInput").value;
    let target = document.getElementById("deviceSelect").value;

    if (msg.trim() === "") return;

    // ✅ Store only the latest message (overwrites in DB but UI keeps history)
    db.ref("/messages/esp" + target).set({
        message: msg,
        timestamp: firebase.database.ServerValue.TIMESTAMP
    });

    document.getElementById("messageInput").value = ""; // Clear input
}

// 🔄 Listen for Only the Most Recent Message (Append to UI Instead of Overwriting)
// 🔄 Listen for Only the Most Recent Message (Append to UI Instead of Overwriting)
db.ref("/messages/espA/message").on("value", (snapshot) => {
    if (snapshot.exists()) {
        let message = snapshot.val();
        appendMessage("ESP32-A: " + message);
    }
});

db.ref("/messages/espB/message").on("value", (snapshot) => {
    if (snapshot.exists()) {
        let message = snapshot.val();
        appendMessage("ESP32-B: " + message);
    }
});


// 📌 Append messages to UI instead of overwriting
function appendMessage(text) {
    let messageList = document.getElementById("messagesList");
    let li = document.createElement("li");
    li.textContent = text;
    messageList.appendChild(li);
}
