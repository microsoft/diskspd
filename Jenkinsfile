pipeline {
  agent {
    node {
      label 'windows_builder'
    }

  }
  stages {
	stage('build VM'){
		build 'WindowsBuilder/master'
	}
    stage('Build DiskSpd Class Library') {
      steps {
        bat(script: 'PATH = %PATH%;"C:\\Program Files (x86)\\Microsoft SDKs\\Windows\\v10.0A\\bin\\NETFX 4.8 Tools"', label: 'Set path for xsd.exe')
        bat(script: 'setx /m Path "%Path%;C:\\Program Files (x86)\\Microsoft SDKs\\Windows\\v10.0A\\bin\\NETFX 4.8 Tools"', label: 'Setting Path')
        bat '"C:\\Program Files (x86)\\Microsoft Visual Studio\\2019\\BuildTools\\MSBuild\\Current\\Bin\\msbuild" /t:Build /p:Configuration=Release /p:Platform="x64" C:\\Jenkins\\workspace\\DiskSpd_master\\diskspd_CLRclassLibrary\\diskspd_CLRclassLibrary.sln'
      }
    }
    stage('Deploy to Nexus') {
      steps {
        bat(script: 'mvn deploy:deploy-file -DgroupId=com.enmotus -DartifactId=diskspd_CLRclassLibrary -Dversion=2.0.21a -DgeneratePom=true -Dpackaging=dll -DrepositoryId=enmotus-nexus -Durl=http://23.99.9.34:8081/repository/maven-releases -Dfile=C:\\Jenkins\\workspace\\DiskSpd_master\\diskspd_CLRclassLibrary\\x64\\Release\\\\diskspd_CLRclassLibrary.dll', returnStatus: true, label: 'mvn deploy:deploy-file')
      }
    }
  }
}