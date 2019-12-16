pipeline {
  agent {
    node {
      label 'windows_builder'
    }

  }
  stages {
    stage('Build DiskSpd Class Library') {
      steps {
        bat '"C:\\Program Files (x86)\\Microsoft Visual Studio\\2019\\BuildTools\\MSBuild\\Current\\Bin\\msbuild /t:Build" /p:Configuration=Release /p:Platform="x64" C:\\Jenkins\\workspace\\DiskSpd_master\\diskspd_CLRclassLibrary\\diskspd_CLRclassLibrary'
      }
    }
  }
}