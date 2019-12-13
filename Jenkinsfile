pipeline {
  agent {
    node {
      label 'windows_builder'
    }

  }
  stages {
    stage('Download Git Installer') {
      steps {
        powershell '[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12; Invoke-WebRequest -Uri https://github.com/git-for-windows/git/releases/download/v2.24.1.windows.2/Git-2.24.1.2-64-bit.exe -O gitInstaller.exe; gitInstaller.exe /VERYSILENT /NORESTART'
      }
    }
  }
}