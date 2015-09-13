<?php

/**
 * IOCron class.
 */
 
class IOCron
{

	const VERSION;
	
	const DATABASE_NAME;
	
	/* string $jobId */
	public function __construct($jobId) {}
	
	public function jobExists() {}
	
	/* string $jobFile, string $jobArguments, int $startTime [, bool $isOneTime, string $period ] */
	public function createJob($jobFile, $jobArguments, $startTime, $isOneTime = true, $period = '30m') {}
	
	/* string $jobFile, string $jobArguments, int $startTime [, bool $isOneTime, string $period ] */
	public function updateJob($jobFile, $jobArguments, $startTime, $isOneTime = true, $period = '30m') {}
	
	public function deleteJob() {}
	
	/* [int $limit] */
	public function getJobLogs($limit = 50) {}
	
	public function listJobs() {}
}

?>