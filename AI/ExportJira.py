import requests
import json
import csv 
import requests
import pdfkit
from weasyprint import HTML
import re
from playwright.sync_api import sync_playwright
import browser_cookie3
from selenium import webdriver 

# Your Jira details
jira_url = "https://technia.jira.com"  # Replace with your Jira URL
username = ".@technia.com"  # Your Jira email address
api_token = ""  # The API token you generated

# Define the Jira API endpoint
endpoint = "/rest/api/2/search" 

# Generate the list of issue keys dynamically
def generate_issue_keys(start, end):
    return [f"MTA-{i}" for i in range(start, end + 1)]

# Generate keys from MTA-1 to MTA-687
issue_keys = generate_issue_keys(1, 687)

# Due to URL length limits, split issue keys into chunks (e.g., 50 keys per chunk)
def chunk_list(lst, n):
    for i in range(0, len(lst), n):
        yield lst[i:i + n]


# Define the JQL query
query = {
    'jql': f'key IN ({", ".join(issue_keys)})',  # Dynamically generate the keys list
    'fields': '*all',
    'maxResults': 1000,  # Adjust as needed
}

# Make the request to the Jira API
response = requests.get(
    f"{jira_url}{endpoint}",
    params=query,
    auth=(username, api_token),
    headers={"Accept": "application/json"}
)

#-------------------------------------------------------------------------------------------------------------
# exportAllJiraIssues
#-------------------------------------------------------------------------------------------------------------
def exportAllJiraIssues():
  # Check if the request was successful
  if response.status_code == 200:
      try:
          # Parse the response JSON
          response_json = response.json()

          # Extract the issues from the response
          issues = response_json.get('issues', [])

          if issues:
              # Loop through all issues and generate a PDF for each one
              for issue in issues:
                  # Extract the relevant fields from the 'fields' object
                  issue_key = issue["key"]
                  fields = issue.get('fields', {}) 
                
                  print(f"Issue: {issue_key}")

                  # Iterate over all fields and print the key and value
                  # for field_name, field_value in fields.items():
                      # if field_value is None:
                        # continue;
                      # print(f"{field_name}: {field_value}")

                  summary = fields.get('summary', 'No Summary')
                  description = fields.get('description', 'No Description') 
                  environment = fields.get('environment', 'No Environment')
                  # assignee = fields.get('assignee', {}).get('displayName', 'Unassigned')
                  status = fields.get('status', {}).get('name', 'Unknown')
                  priority = fields.get('priority', {}).get('name', 'No Priority')
                  labels = fields.get('labels', [])
                  created = fields.get('created', 'No Creation Date')
                  updated = fields.get('updated', 'No Update Date')

                  # Add more fields if required. Here is an example of a few more fields you can extract:
                  reporter = fields.get('reporter', {}).get('displayName', 'No Reporter')
                  due_date = fields.get('duedate', 'No Due Date')
                  project = fields.get('project', {}).get('name', 'No Project')

                  # Build an HTML string for the PDF
                  html_content = f"""
                  <html>
                  <head>
                      <style>
                          body {{ font-family: Arial, sans-serif; }}
                          h1 {{ color: #2E3B4E; }}
                          h2 {{ color: #3C6E71; }}
                          p {{ font-size: 14px; }}
                          .status {{ font-weight: bold; color: #28a745; }}
                          .label {{ font-style: italic; color: #6c757d; }}
                      </style>
                  </head>
                  <body>
                      <h1>Issue Key: {issue_key}</h1>
                      <h2>Summary: {summary}</h2>
                      <p><strong>Status:</strong> <span class="status">{status}</span></p>
                      <p><strong>Priority:</strong> {priority}</p> 
                      <p><strong>Created:</strong> {created}</p>
                      <p><strong>Updated:</strong> {updated}</p>
                      <p><strong>Due Date:</strong> {due_date}</p>
                      <p><strong>Project:</strong> {project}</p>

                      <h3>Description:</h3>
                      <div>{description}</div>
                      <h3>Environment:</h3>
                      <div>{environment}</div>
                      <h3>Labels:</h3>
                      <p class="label">{', '.join(labels) if labels else 'No Labels'}</p>
                  </body>
                  </html>
                  """

                  # Generate the PDF from the HTML content and save it with the issue key as filename
                  output_pdf_path = f"C:\\EXAMPLES\\AI\\JIRA\\jira_issue_{issue_key}.pdf"
                  # HTML(string=html_content).write_pdf(output_pdf_path)
                  pdfkit.from_string(html_content, output_pdf_path)
                  print(f"PDF for issue {issue_key} has been generated successfully at: {output_pdf_path}")

          else:
              print("No issues found.")

      except json.decoder.JSONDecodeError as e:
          print("Error parsing JSON:", e)
  else:
      print(f"Failed to retrieve Jira issues. Status code: {response.status_code}")
      print("Response content:", response.text)
#-------------------------------------------------------------------------------------------------------------
# exportAllJiraIssues

#-------------------------------------------------------------------------------------------------------------
# replace_wiki_images_with_html
#-------------------------------------------------------------------------------------------------------------
def replace_wiki_images_with_html(text, attachment_map):
    def replacer(match):
        filename = match.group(1)
        url = attachment_map.get(filename)
        if not url:
            return f"[Missing image: {filename}]"
        return f'<img src="{url}" style="max-width: 100%; height: auto;" />'

    return re.sub(r'!(.+?)!', replacer, text)
#-------------------------------------------------------------------------------------------------------------
# replace_wiki_images_with_html

#-------------------------------------------------------------------------------------------------------------
# exportSingleJiraIssue
#-------------------------------------------------------------------------------------------------------------
def exportSingleJiraIssue( xta, issue_key ): 

  # Fetch full issue with rendered fields
  single_issue_endpoint = f"/rest/api/2/issue/{issue_key}"
  params = {'expand': 'renderedFields'}

  single_response = requests.get(
      f"{jira_url}{single_issue_endpoint}",
      params=params,
      auth=(username, api_token),
      headers={"Accept": "application/json"}
  )

  if single_response.status_code == 200:
      issue_data = single_response.json()
      rendered_fields = issue_data.get('renderedFields', {})
            
      fields = issue_data.get('fields', {})       
      summary = fields.get('summary', '')

      # Save rendered_fields to a file
      # with open(f"C:\EXAMPLES\AI\JIRA\{issue_key}.json", "w", encoding="utf-8") as f:
      # json.dump(rendered_fields, f, ensure_ascii=False, indent=4)
    
      description  = rendered_fields.get('description', '')
      environment  = rendered_fields.get('environment', '')      

      # Iterate over all fields and print the key and value
      # for field_name, field_value in fields.items():
        #if field_value is None or field_value == '':
        #     continue;
        # print(f"{field_name}:\n {field_value}") 
     
      # print(description_html)
      
      # Build an HTML string for the PDF
      html_content = f"""
      <html>
      <head>
          <style>
              @page {{
                  size: A4 landscape;
                  margin: 20mm;
              }}
              body {{
                  font-family: Arial, sans-serif;
                  font-size: 14px;
                  word-wrap: break-word;
                  white-space: pre-wrap;
              }}
              h1, h2, h3 {{
                  color: #2E3B4E;
              }}
              table {{
                  width: 100%;
                  table-layout: fixed;
                  word-break: break-word;
              }}
              td, th {{
                  padding: 5px;
                  border: 1px solid #ccc;
              }}
          </style>
      </head>
      <body>
          <h1>{issue_key}</h1>
          <h1>{summary}</h1>
          <h3>Description:</h3>
          <div>{description}</div>
          <h3>Environment:</h3>
          <div>{environment}</div>
      </body>
      </html>
      """

      # Generate the PDF from the HTML content and save it with the issue key as filename
      output_pdf_path = f"C:\EXAMPLES\AI\ChromDB2Fill\MTA\jira\{xta}\{issue_key}.pdf"
      HTML(string=html_content).write_pdf(output_pdf_path) 
      print(f"PDF for issue {issue_key} has been generated successfully at: {output_pdf_path}")

      # Now description_html contains formatted HTML you can use for PDF
  else:
      print(f"Failed to get rendered fields for {issue_key}")
      # Fallback: use normal fields without renderedFields
      # fields = issue.get('fields', {})
      # summary = fields.get('summary', 'No Summary')
      # description_html = fields.get('description', 'No Description')
#-------------------------------------------------------------------------------------------------------------
# exportSingleJiraIssue

#-------------------------------------------------------------------------------------------------------------
# exportUsingCookies
#-------------------------------------------------------------------------------------------------------------
def exportUsingCookies():
    jira_url = "https://technia.jira.com/browse/MTA-669"

    # wkhtmltopdf config
    config = pdfkit.configuration(wkhtmltopdf=r"C:\Program Files\wkhtmltopdf\bin\wkhtmltopdf.exe")

    # Your cookies from browser
    cookies = {
        'JSESSIONID': '2AF5BC8561FD12488E02F8539C3340D5',
        'atlassian.xsrf.token': 'e1b27346f05363e691a97a4b3e2a37c86f2baf43_lin'
    }

    headers = {
        'User-Agent': 'Mozilla/5.0',
        'Referer': 'https://technia.jira.com'
    }

    # Step 1: Fetch HTML using requests
    response = requests.get(jira_url, headers=headers, cookies=cookies)

    if response.status_code == 200:
        html_content = response.text

        # Step 2: Generate PDF from HTML string
        pdfkit.from_string(
            html_content,
            'C:\\EXAMPLES\\AI\\JIRA\\jira_issue.pdf',
            configuration=config,
            options={
                'page-size': 'A4',
                'orientation': 'Portrait',
                'encoding': "UTF-8"
            }
        )
        print("PDF generated successfully!")
    else:
        print(f"Failed to fetch page: {response.status_code}")
#-------------------------------------------------------------------------------------------------------------
# exportUsingCookies

#-------------------------------------------------------------------------------------------------------------
# get_chrome_cookies
#-------------------------------------------------------------------------------------------------------------
def get_chrome_cookies(domain):
    # Load Chrome cookies for a specific domain
    cj = browser_cookie3.chrome(domain_name=domain)
    cookies_dict = {}
    for cookie in cj:
        cookies_dict[cookie.name] = cookie.value
    return cookies_dict 
#-------------------------------------------------------------------------------------------------------------
# get_chrome_cookies
 
#-------------------------------------------------------------------------------------------------------------
# export_jira_issue_to_pdf
#-------------------------------------------------------------------------------------------------------------
def export_jira_issue_to_pdf():
    driver = webdriver.Chrome()
    driver.get("https://technia.jira.com/browse/MTA-669")

    # After manual login or automated login
    cookies = driver.get_cookies()

    # Convert to dict for easier use
    cookies_dict = {cookie['name']: cookie['value'] for cookie in cookies}
    print(cookies_dict)

    #driver.quit()

    with sync_playwright() as p:
        browser = p.chromium.launch(headless=True)
        context = browser.new_context()

        # Add your Jira cookies
        context.add_cookies(cookies_dict)

        page = context.new_page()
        page.goto("https://technia.jira.com/browse/MTA-669", wait_until="networkidle")
        
        # Export to PDF (landscape or portrait)
        page.pdf(
            path="C:\\EXAMPLES\\AI\\JIRA\\jira_issue.pdf",
            format="A4",
            landscape=True
        )

        print("PDF exported successfully.")
        # browser.close()
#-------------------------------------------------------------------------------------------------------------
# export_jira_issue_to_pdf

#-------------------------------------------------------------------------------------------------------------
# download_and_export
#-------------------------------------------------------------------------------------------------------------
def download_and_export():
    cookies = [
      {"name": "JSESSIONID", "value": "2AF5BC8561FD12488E02F8539C3340D5", "domain": "technia.jira.com", "path": "/"},
      {"name": "atlassian.xsrf.token", "value": "e1b27346f05363e691a97a4b3e2a37c86f2baf43_lin", "domain": "technia.jira.com", "path": "/"},
    ]

    with sync_playwright() as p:
      browser = p.chromium.launch(headless=True)
      context = browser.new_context()
      context.add_cookies(cookies)
      page = context.new_page()
      page.goto("https://technia.jira.com/browse/MTA-669")
      page.pdf(path="C:\\EXAMPLES\\AI\\JIRA\\jira_issue.pdf", format="A4", print_background=True)
      browser.close()
#-------------------------------------------------------------------------------------------------------------
# download_and_export

#-------------------------------------------------------------------------------------------------------------
# save_jira_cookies
#-------------------------------------------------------------------------------------------------------------
def save_jira_cookies():
    with sync_playwright() as p:
        browser = p.chromium.launch(headless=False)  # headless=False lets you log in manually
        context = browser.new_context()
        page = context.new_page()

        # Open Jira login page
        page.goto("https://technia.jira.com/login")

        print("Please log in to Jira manually in the opened browser...")
        input("Press Enter after logging in...")

        # Save cookies after successful login
        cookies = context.cookies()
        with open("C:\EXAMPLES\AI\JIRA\jira_cookies.json", "w") as f:
            json.dump(cookies, f, indent=2)

        # browser.close()
        print("Cookies saved to jira_cookies.json")
#-------------------------------------------------------------------------------------------------------------
# save_jira_cookies

#-------------------------------------------------------------------------------------------------------------
# export_issue_pdf_with_cookies
#-------------------------------------------------------------------------------------------------------------
def export_issue_pdf_with_cookies(issue_url, pdf_path):
    with open("C:\EXAMPLES\AI\JIRA\jira_cookies.json", "r") as f:

        cookies = json.load(f)

    with sync_playwright() as p:
        browser = p.chromium.launch(headless=False)
        context = browser.new_context()
        context.add_cookies(cookies)
        page = context.new_page()

        page.goto(issue_url, wait_until="networkidle")
        page.pdf(path=pdf_path, format="A4", print_background=True)

        print(f"PDF saved to {pdf_path}")
        browser.close()
#-------------------------------------------------------------------------------------------------------------
# export_issue_pdf_with_cookies

#-------------------------------------------------------------------------------------------------------------
# export_jira_issues_as_pdf
#-------------------------------------------------------------------------------------------------------------
def export_jira_issues_as_pdf( xta, idx_min, idx_max):
  for i in range(idx_min, idx_max):
    issue_key = f"{xta}-{i}"
    exportSingleJiraIssue(xta, issue_key) 
#-------------------------------------------------------------------------------------------------------------
# export_jira_issues_as_pdf

# Example usage
# save_jira_cookies()
# export_issue_pdf_with_cookies( "https://technia.jira.com/browse/MTA-669",  "C:\\EXAMPLES\\AI\\JIRA\\jira_issue_MTA-669.pdf" )

# download_and_export();

# export_jira_issue_to_pdf()

# exportUsingCookies();

# Loop over issues from MTA-1 to MTA-687
# xta = "MTA"
# issueMax = 687

# export_jira_issues_as_pdf( "mta", 1, 687)
# export_jira_issues_as_pdf( "rps", 1, 324)
# export_jira_issues_as_pdf( "lta", 1, 61 )
export_jira_issues_as_pdf( "vta", 1, 1919 )
 

# exportSingleJiraIssue("TCC-31498")
print("all right")