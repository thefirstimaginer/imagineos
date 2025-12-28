function openPage(pageName, element, color) {
    // Hide all elements with class="tabcontent" by default */
    var i, tabcontent, tablinks;

    // Get all elements with class="tabcontent and hide them
    tabcontent = document.getElementsByClassName("tabcontent");
        for (i = 0; i < tabcontent.length; i++) {
        tabcontent[i].style.display = "none";
    };
    
    // Remove the background color of all tablinks/buttons
    tablinks = document.getElementsByClassName("tablink");
        for (i = 0; i < tablinks.length; i++) {
        tablinks[i].style.backgroundColor = "";
        tablinks[i].style.color = "";
    
    }
    
    // Show the specific tab content
    document.getElementById(pageName).style.display = "grid";
    
    // Add the specific color to the button used to open the tab content
    element.style.backgroundColor = "rgba(56, 189, 248, 0.1)";
    element.style.color = "#38bdf8";
}

// Get the element with id="defaultOpen" and click on it
document.getElementById("defaultOpen").click();