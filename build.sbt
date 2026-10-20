// Copyright 2026 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// CoralNPU Chisel generators and unit tests.
//
// Replaces the rules_scala/rules_jvm_external setup of the old Bazel build.
// Third-party RTL (common_cells, cvfpu, fpu_div_sqrt_mvp, RVVI) comes from
// `bender vendor` (see Bender.yml); Chisel BlackBoxes still locate it as
// classpath resources, so it is staged below under the same paths the Scala
// sources have always used (external/<pkg>/..., hdl/verilog/...).

ThisBuild / scalaVersion := "2.13.18"
ThisBuild / organization := "com.google.coralnpu"
ThisBuild / version := "0.0.1"

val chiselVersion = "7.0.0-RC1"

val srcRoot = file("hdl/chisel/src")

// A source file is a test iff it pulls in scalatest. Helpers that do not
// (e.g. *TestUtils, *Testbench) stay in the main configuration because
// generators may depend on some of them.
def isTestSource(f: File): Boolean =
  IO.read(f).contains("org.scalatest")

val chiselDeps = Seq(
  addCompilerPlugin("org.chipsalliance" % "chisel-plugin_2.13.6" % chiselVersion),
  libraryDependencies += "org.chipsalliance" %% "chisel" % chiselVersion,
)

// rocket-chip's AsyncQueue (and the little it needs), vendored as Scala sources.
lazy val asyncqueue = (project in file("build/asyncqueue"))
  .settings(chiselDeps)
  .settings(
    name := "asyncqueue",
    scalacOptions += "-nowarn",
    Compile / unmanagedSourceDirectories := Nil,
    Compile / unmanagedSources :=
      (file("vendor/rocket_chip/src/main/scala/util") * "*.scala").get ++
        Seq(file("vendor/diplomacy/diplomacy/src/diplomacy/nodes/HeterogeneousBag.scala")),
  )

lazy val root = (project in file("."))
  .dependsOn(asyncqueue)
  .settings(
    name := "coralnpu",
    scalacOptions ++= Seq(
      "-Ymacro-annotations",
      "-explaintypes",
      "-feature",
      "-language:reflectiveCalls",
      "-unchecked",
      "-deprecation",
      "-Xcheckinit",
      "-Xlint:infer-any",
      "-Xlint:unused",
    ) ++ (if (sys.env.contains("SCALA_FATAL_WARNINGS")) Seq("-Xfatal-warnings") else Nil),

    // The chisel plugin artifact is the one the Bazel build used (_2.13.6).
    addCompilerPlugin("org.chipsalliance" % "chisel-plugin_2.13.6" % chiselVersion),
    libraryDependencies ++= Seq(
      "org.chipsalliance" %% "chisel" % chiselVersion,
      "org.chipsalliance" %% "firtool-resolver" % "2.0.0",
      "org.scala-lang" % "scala-reflect" % scalaVersion.value,
      "com.github.scopt" %% "scopt" % "3.7.1",
      "net.jcazevedo" %% "moultingyaml" % "0.4.2",
      "org.apache.commons" % "commons-lang3" % "3.11",
      "org.apache.commons" % "commons-text" % "1.10.0",
      "org.scalatest" %% "scalatest" % "3.2.16" % Test,
      "edu.berkeley.cs" %% "firrtl" % "5.0.0" % Test,
      "org.antlr" % "antlr4-runtime" % "4.13.1" % Test,
      "net.java.dev.jna" % "jna" % "5.14.0" % Test,
    ),

    // All sources live under hdl/chisel/src.
    Compile / unmanagedSourceDirectories := Nil,
    Compile / unmanagedSources :=
      (srcRoot ** "*.scala").get.filterNot(isTestSource),
    Test / unmanagedSourceDirectories := Nil,
    Test / unmanagedSources :=
      (srcRoot ** "*.scala").get.filter(isTestSource),

    // coralnpu.ScmInfo: the git revision, as the old stamped Bazel build did.
    Compile / sourceGenerators += Def.task {
      import scala.sys.process._
      val rev = scala.util.Try(Seq("git", "rev-parse", "HEAD").!!.trim)
        .toOption.filter(_.matches("[0-9a-f]{40}")).getOrElse("f" * 40)
      val f = (Compile / sourceManaged).value / "coralnpu" / "ScmInfo.scala"
      IO.write(f,
        s"""package coralnpu
           |
           |class ScmInfo {
           |    val revision = BigInt("$rev", 16)
           |}
           |""".stripMargin)
      Seq(f)
    }.taskValue,

    // Stage RTL resources for BlackBoxes (HasBlackBoxResource.addResource).
    Compile / resourceGenerators += Def.task {
      val root = baseDirectory.value
      val out = (Compile / managedResourceDirectories).value.head
      val rtl = "*.sv" | "*.svh" | "*.v"
      // (classpath prefix, source directory)
      val mappings = Seq(
        "external/common_cells" -> root / "vendor/common_cells",
        "external/cvfpu" -> root / "vendor/cvfpu",
        "external/fpu_div_sqrt_mvp" -> root / "vendor/fpu_div_sqrt_mvp",
        "external/RVVI" -> root / "vendor/RVVI",
        "hdl/verilog" -> root / "hdl/verilog",
        // hdl/verilog/{ClockGate,RstSync,Sram} are also addressed bare.
        "" -> root / "hdl/verilog",
      )
      mappings.flatMap { case (prefix, dir) =>
        val finder =
          if (prefix.isEmpty) (dir * rtl) else (dir ** rtl)
        finder.get.filter(_.isFile).map { f =>
          val rel = IO.relativize(dir, f).get
          val dst = if (prefix.isEmpty) out / rel else out / prefix / rel
          IO.copyFile(f, dst)
          dst
        }
      }
    }.taskValue,

    Test / fork := true,
    Test / testOptions += Tests.Argument(TestFrameworks.ScalaTest, "-oF"),
    run / fork := true,
  )
