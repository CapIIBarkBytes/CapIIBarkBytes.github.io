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



// Initial page load (optional): Show the home page by default
document.getElementById('home').classList.add('active');
