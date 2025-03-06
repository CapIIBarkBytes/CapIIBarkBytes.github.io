// script.js
const burger = document.querySelector('.burger');
const navLinks = document.querySelector('.nav-links');
const navItems = document.querySelectorAll('.nav-item');
const pages = document.querySelectorAll('.page');

burger.addEventListener('click', () => {
    navLinks.classList.toggle('active');
    navItems.forEach((link, index) => {
        if (link.style.animation) {
            link.style.animation = '';
        } else {
            link.style.animation = `navLinkFade 0.5s ease forwards ${index / 7 + 0.5}s`; // Added animation
        }
    });
});

navLinks.addEventListener('click', (event) => {
    if (event.target.tagName === 'A') {
        event.preventDefault(); // Prevent default link behavior
        const targetPage = event.target.dataset.page;

        // Hide all pages
        pages.forEach(page => page.classList.remove('active'));

        // Show the selected page
        document.getElementById(targetPage).classList.add('active');

        // Close the menu on smaller screens
        if (window.innerWidth <= 768) {
            navLinks.classList.remove('active');
        }
    }
});
const dispenseButtons = document.querySelectorAll('.dispense-button');
dispenseButtons.forEach(button => {
    button.addEventListener('click', () => {
        const amount = button.dataset.amount;
        const url = `https://famous-smoothly-sunbeam.ngrok-free.app/feed?amount=${amount}`;

        // Make the HTTPS request (using the fetch API or XMLHttpRequest)
        fetch(url)
            .then(response => {
                // Handle the response from the server
                if (response.ok) {
                    alert(`Dispensing ${amount} amount!`);
                } else {
                    alert(`Error: ${response.status}`);
                }
            })
            .catch(error => {
                console.error('Error:', error);
                alert('An error occurred while making the request.');
            });
    });
});


// Initial page load (optional): Show the home page by default
document.getElementById('home').classList.add('active');
