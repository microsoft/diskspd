library identifier: 'EnmotusBuildTools@master', retriever: modernSCM([$class: 'GitSCMSource', credentialsId: 'enmotusdavecohen', gitTool: 'master_git', remote: 'https://github.com/Enmotus-Dave-Cohen/EnmotusBuildTools.git', traits: [[$class: 'jenkins.plugins.git.traits.BranchDiscoveryTrait'], [$class: 'GitToolSCMSourceTrait', gitTool: 'master_git']]])
pipeline {
  agent {
    node {
      label 'windows_builder'
		}
	}
  stages {
    stage('Call Library') {
		steps {
			checkout changelog: false, poll: false, scm: [$class: 'GitSCM', branches: [[name: '*/master']], doGenerateSubmoduleConfigurations: false, extensions: [], gitTool: 'default', submoduleCfg: [], userRemoteConfigs: [[credentialsId: 'enmotusdavecohen', url: 'https://github.com/Enmotus-Dave-Cohen/WindowsBuilder.git']]]
			script {
			InstallWindowsBuilderDependencies.call()
			}
                    
		}
	stage('Update Version'){
		steps {
			script {
                    if(currentBuild.changeSets.size() > 0) {
                        echo "version number needs to be updated"
                    }
                    else {
                        echo "there are no changes in this build"
                    }
			}
		}
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