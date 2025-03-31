document.addEventListener("DOMContentLoaded", function () {
    document.querySelector("form").addEventListener("submit", async function (e) {
        e.preventDefault(); // Prevent default form submission script
        
        // Retrieve form data
        const scheduleName = document.getElementById("name").value;
        const petName = document.getElementById("petname").value;
        const applyImmediately = document.getElementById("immapply").checked;
        const foodAmount = document.getElementById("food").value;
        
        // Get selected days
        const selectedDays = [...document.querySelectorAll("input[name='days']:checked")].map(day => day.value);
        
        // Get feeding times
        const feedingTimes = [...document.querySelectorAll("input[name='times[]']")].map(timeInput => timeInput.value);
        
        // Using placeholder names for the request data
        // Final schedule may be using different names
        const requestData = {
            schedule_name: scheduleName,
            dog_name: petName,
            apply_immediately: applyImmediately,
            days: selectedDays,
            food_amount: foodAmount,
            feeding_times: feedingTimes
        };
        
        try {
            const response = await fetch("https://famous-smoothly-sunbeam.ngrok-free.app/schedule", {
                method: "POST",
                headers: { "Content-Type": "application/json"},
                body: JSON.stringify(requestData)
            });
            
            if (!response.ok) {
                throw new Error("Failed to submit schedule! Plese check your input.");
            }
            
            const result = await response.json();
            alert("Schedule submitted successfully!");
            console.log("Response:", result);
        } catch (error) {
            console.error("Error:", error);
            alert("Error submitting schedule");
        }
    });
});
