properties([[$class: 'BuildDiscarderProperty', strategy: [$class: 'LogRotator', artifactNumToKeepStr: '2', numToKeepStr: '2']]])

node("master") {
    def projectName = "ftl-sdk"
    
    def projectDir = pwd()+ "/${projectName}"
    def artifactDir = "${projectDir}/build"

    try {
        sh "mkdir -p '${projectDir}'"
        dir (projectDir) {
            stage("Checkout") {
                checkout scm
            }
            stage("submodules") {
                sh 'git submodule update --init'
            }
            stage("make all") {
                sh "mkdir -p build && cd build && cmake .. && make"
            }
            stage("deploy") {
                archiveArtifacts artifacts: "build/ftl_app,build/libftl.so*", fingerprint: false
            }
            currentBuild.result = "SUCCESS"
        }
    } catch(e) {
        currentBuild.result = "FAILURE"
        throw e
    }
}
