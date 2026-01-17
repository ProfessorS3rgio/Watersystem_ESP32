<?php

namespace App\Http\Controllers;

use App\Models\BarangaySequence;
use Illuminate\Http\Request;

class BarangaySequenceController extends Controller
{
    public function index()
    {
        return response()->json([
            'data' => BarangaySequence::all()
        ]);
    }
}