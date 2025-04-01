// js handling form login page 
document.addEventListener("DOMContentLoaded", () => {
  const loginForm = document.getElementById("login-form");

  loginForm.addEventListener("submit", async (logevent) => {
    logevent.preventDefault();

    const logUsername = document.getElementById("log-username").value.trim();
    const logPassword = document.getElementById("log-password").value.trim();

    if (!logUsername || !logPassword) {
        alert("Please fill in both username and password.");
    } else {
      try {
          const logChecking = await fetch('https://famous-smoothly-sunbeam.ngrok-free.app/login', {
            method:'POST',
            headers: {
              'Content-Type': 'application/json'
            },
            body: JSON.stringify({
              username: logUsername,
              password: logPassword
            })
          });
          const data = await logChecking.json();
          
          if(logChecking.ok) {
            window.location.href = "index.html";
          } else {
            alert("Invalid username or password");
          }
        } catch (error) {
          alert("Having trouble connecting to server...Please try again.");
        }
      }
  })
})

// js handling form sign up page 
document.addEventListener("DOMContentLoaded", () => {
  const signForm = document.getElementById("sign-form");

  signForm.addEventListener("submit", async (signevent) => {
    signevent.preventDefault();

    const signUsername = document.getElementById("sign-username").value.trim();
    const signPassword = document.getElementById("sign-password").value.trim();

    if (!signUsername || !signPassword) {
      alert("Please fill in both username and password.");
    } else {
      try {
        const signChecking = await fetch('https://famous-smoothly-sunbeam.ngrok-free.app/login', { 
          method: 'POST', 
          headers: {
            'Content-Type': 'application/json'
          },
          body: JSON.stringify({
            username: signUsername,
            password: signPassword
          })
        });
        
        const data = await signChecking.json();
        if (signChecking.ok) {
          alert("Account has been successfully created!");
          window.location.href = 'index.html';
        } else {
          alert("Signup failed. Account may already be registered. Please try again.");
        }
      } catch (error) {
        alert("Having trouble connecting to the server...Please try again.");
      }
    }
  })
})