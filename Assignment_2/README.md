# DNS Resolver - Iterative and Recursive Lookup

## Implementation Details
- The script uses the **dnspython** library for querying DNS servers.
- **Iterative resolution** starts at the root servers and follows the DNS hierarchy (Root → TLD → Authoritative servers).
- **Recursive resolution** relies on the system's resolver to fetch the IP directly.
- Includes robust **error handling** for timeouts, non-existent domains, and unreachable servers.
- Supports **command-line execution** for ease of use.
- Performance measurement is included to display execution time.

## Output Format
- **Iterative lookup** shows debugging information about each stage (ROOT, TLD, AUTH).
- **Recursive lookup** displays resolved IP addresses along with authoritative name servers.
- Errors are clearly marked with `[ERROR]` tags for easy identification.
- Displays total time taken for the resolution process.

## Requirements Met
- Supports command-line arguments for iterative and recursive lookups.
- Provides detailed debugging output for iterative resolution.
- Implements error handling for timeouts, non-existent domains, and other failures.
- Clean and well-documented code with comments explaining functionality.

## Sources

The following references and materials were used in building this DNS Resolver:

- **dnspython** Docs - https://dnspython.readthedocs.io/en/stable/manual.html
- YouTube - https://youtu.be/SLQrbjeVrk0?feature=shared

## Contribution
### **Team Members**
- Daksh Kumar Singh-220322
- Himanshu Shekhar-220454
- Swayamsidh Pradhan-221117

The work was equally distributed among all the three team members, with each member contributing 33.33% of the total effort through collaborative implementation, testing, and debugging of the project.

## Declaration

This project is developed as part of the **IITK CS425 Computer Networks** course under **Prof. Aditya Vadapalli**.  
All code and documentation are intended for educational purposes only. Any external resources used have been appropriately referenced in the **Sources** section.  


## Feedback

For any queries or feedback, feel free to reach out via email. We appreciate your input and are happy to assist with any questions!
